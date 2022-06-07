obj-m += MyDriver.o

all:
	make -C /lib/modules/5.15.32-v7l+/build M=$(PWD) modules

clean:
	make -C /lib/modules/5.15.32-v7l+/build M=$(PWD) clean

