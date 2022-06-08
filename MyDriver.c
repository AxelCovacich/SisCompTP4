#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Axel Covacich y Mateo Merino. UNC-Fcefyn");
MODULE_DESCRIPTION("Sensor de humedad y temperatura");


#define MAXTIMINGS  160
#define BUFF_LEN 80
#define HIGH 1
#define LOW 0

#define NAME_REGION "sensor_char"
#define NAME_CLASS "sensor_class"
#define NAME_DEVICE "sensor"


static dev_t  dev_num;      //contiene ambos numeros major y minor
static struct cdev c_dev; 	// Global variable for the character device structure
static struct class *cl; 	// Global variable for the device class
static char channel = '0';
static int sensordata[5] = { 0, 0, 0, 0, 0 };
static int SENSOR_PIN = 23;
static int cont = 1;

static int read_sensor_data(void)
{
    uint8_t laststate   = HIGH;
    uint8_t counter     = 0;
    uint8_t j       = 0, i;
 
    sensordata[0] = sensordata[1] = sensordata[2] = sensordata[3] = sensordata[4] = 0; 
    
    // Make a GPIO an output, and set its value at LOW 
    gpio_direction_output(SENSOR_PIN, LOW);
    
    // pull pin down for 18 milliseconds 
   
    usleep_range(18000,20000);
    // then pull it up for 40 microseconds 
    gpio_set_value(SENSOR_PIN, HIGH);
    
    udelay(40);
    // prepare to read the pin
    gpio_direction_input(SENSOR_PIN);
    
    //detect change and read data 
    
    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        while (gpio_get_value(SENSOR_PIN) == laststate)
        {
            counter++;
            udelay(1);
            if (counter == 255)
            {
                break;
            }
        }
        laststate = gpio_get_value(SENSOR_PIN);
 
        if (counter == 255)
            break;
        
        // ignore first 3 transitions 
        
        if ( (i >= 4) && (i % 2 == 0) )
        {
            // shove each bit into the storage bytes 
            sensordata[j / 8] <<= 1;
            if ( counter > 16 )
                sensordata[j / 8] |= 1;
            j++;
        }
    }
    
     // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
     // print it out if data is good 
     
    if ((j >= 40) &&(sensordata[4] == ((sensordata[0] + sensordata[1] + sensordata[2] + sensordata[3]) & 0xFF)))
    {
	printk(KERN_INFO "Humidity = %d.%d %% Temperature = %d.%d °C \n",
            sensordata[0], sensordata[1], sensordata[2], sensordata[3]);

        printk(KERN_INFO "Data OK\n");
        return 1;        
    }
    else
    {
        return 0;
    }
}

static int sensor_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Sensor file opened\n");
    return 0;
}
static int sensor_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Sensor file closed\n");
    return 0;
}
static ssize_t sensor_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{  
    printk(KERN_INFO "Reading file\n");
    if (cont == 1)
    {        
  
        char buff_aux[BUFF_LEN];
        int length = 0;
        
         /* Se guarda el valor obtenido a buff_aux */
        printk(KERN_INFO"Sensing data\n");
        while (read_sensor_data() == 0)
        {
        }   
        
        memset(buff_aux,'\0',BUFF_LEN);
        switch (channel)
        {
            case '0': //Humedad 
                
                sprintf (buff_aux,"%d.%d", sensordata[0], sensordata[1]);
                length = strlen (buff_aux);
                break;

            case '1': // Temperatura

                sprintf (buff_aux, "%d.%d", sensordata[2], sensordata[3]);            
                length = strlen (buff_aux);
                break;

            case '2': // Temperatura y humedad

                sprintf (buff_aux, "%d.%d,%d.%d",sensordata[0], sensordata[1], sensordata[2], sensordata[3]);
                length = strlen (buff_aux);
                break;

            default:

                printk(KERN_INFO "Invalid Option\n");
                return 0;
        }  

       
        if (len < length)
            length = len;
        

        /* Se envia el valor al dispositivo de caracter */
        if (*off == 0)
        {
            if (copy_to_user (buf, buff_aux, length) != 0)
                return -EFAULT;
            
            else {
                cont = 2;                
                (*off)++;
                return length;
            }
            
        }
        return 0;     
    }
    
    cont = 1;
    return 0;
}
static ssize_t sensor_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    printk(KERN_INFO "Writing sensor dev file\n");

    if (copy_from_user(&channel,buf, len) != 0)
        return -EFAULT;
 
    else
    {
    	printk(KERN_INFO "Channel: %c",channel);
        return len;
    }
}

static struct file_operations pugs_fops =
{
    .owner = THIS_MODULE,
    .open = sensor_open,
    .release = sensor_close,
    .read = sensor_read,
    .write = sensor_write
};

static int modulosensor_init(void)
{
    int ret;
    struct device *dev_ret;

	/* el so elige el numero mayor del modulo*/
    if ((ret = alloc_chrdev_region(&dev_num, 0, 1, NAME_REGION)) < 0) return ret;
    
    printk(KERN_INFO "Driver module registered succesfully \n");
    printk(KERN_INFO "Major = %d Minor = %d \n", MAJOR(dev_num), MINOR(dev_num));

    /*Crea una clase en /sys/class*/
    if (IS_ERR(cl = class_create(THIS_MODULE, NAME_CLASS)))
    {
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(cl);
    }

    /* se crea el dispositivo en /dev */
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev_num, NULL, NAME_DEVICE)))
    {
        class_destroy(cl);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(dev_ret);
    }
    cdev_init(&c_dev, &pugs_fops);  //inicializo la estructura c_dev que es la que maneja las funciones del character device

/*
Agrega el CDD al sistema, Aquí, dev es la estructura cdev , num es el primer número 
de dispositivo al que responde este dispositivo (major) y count es la cantidad de números de dispositivo 
que deben asociarse con el dispositivo (minor).
*/
    if ((ret = cdev_add(&c_dev, dev_num, 1)) < 0)
    {
        device_destroy(cl, dev_num);
        class_destroy(cl);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

	/* reserva el pin para sensar la humedad y temperatura */
	ret = gpio_request(SENSOR_PIN, "sensor");

	if (ret)
	{
		printk(KERN_ERR "Unable to request GPIOs for SENSOR_PIN: %d\n", ret);
	}

    return 0;

}

static void modulosensor_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, dev_num);
    class_destroy(cl);
    unregister_chrdev_region(dev_num, 1);
    gpio_free(SENSOR_PIN);

    printk(KERN_INFO "Removed sensor module from kernel.\n");
}

module_init(modulosensor_init);
module_exit(modulosensor_exit);