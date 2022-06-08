import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import random
import matplotlib.animation as animation

from datetime import datetime
from matplotlib import pyplot
from matplotlib.widgets import Button

plt.style.use(['seaborn-whitegrid'])

fig, (ax1, ax2) = plt.subplots(1,2,figsize=(10,10))
plt.subplots_adjust(bottom=0.13)

DEVICE_FILE = "/dev/sensor"

file = open(DEVICE_FILE, "w")
file.write("0")
file.close()

humidity = 0
temperature = 0
channel = 0
time = 0

line1, = ax1.plot([], [],'-', lw=3, color ='red') 
line2, = ax2.plot([], [],'-',lw=3, color='red')
line = [line1, line2]

x_data, y1_data, y2_data  =[], [], []



def plotter(frame):
    global humidity, temperature

    if channel == 0: #sensa humidity

        humidity = read_one_sensor()
        temperature = 0
        x_data.append(time)
        y1_data.append(humidity)
        y2_data.append(temperature)
        print("humidity: ",humidity , "%")

    if channel == 1: # sensa temperature 

        temperature = read_one_sensor()
        humidity = 0
        x_data.append(time)
        y1_data.append(humidity)
        y2_data.append(temperature)
        print("temperature: ",temperature , "°c")
         
    if channel == 2: # sensa ambos canales

        humidity, temperature = read_both_sensors()  
        x_data.append(time)
        y1_data.append(humidity)
        y2_data.append(temperature)
        print("humidity: " , humidity , "% | temperature: ", temperature, "°c")

    line[0].set_data(x_data, y1_data)    
    line[1].set_data(x_data, y2_data)    
       

    return line

def read_both_sensors():
    global time

    time += 1

    file = open(DEVICE_FILE,"r")
    value = file.read() 
    file.close()

    value_sensor = value.split(",")    
        
    hum = float(value_sensor[0])
    temp = float(value_sensor[1])

    return hum, temp

def read_one_sensor():
    global time

    time += 1
    file = open(DEVICE_FILE,"r")
    value = float(file.read()) 
    file.close()

    return value


def set_figure():
    
    fig.set_size_inches(14,14)
    #fig.suptitle('Humidity-Temperature Sensor', fontsize=20 , color='blue')

    #ax1.set_title('Humidity sensor',fontdict={'color':'blue','weight':'bold','size' : 13}, pad=14)
    ax1.set_xlabel('Time[s]',fontdict={'color':'black','weight':'bold','size' : 9})
    ax1.set_ylabel('Humidity[%]',fontdict={'color':'black','weight':'bold','size' : 13})
     
    ax1.tick_params(axis='both', which='major', labelsize=13)
    ax1.tick_params(axis='both', which='minor', labelsize=11)
    ax1.set_yticks(np.arange(0, 100, 5))
    ax1.set_xticks(np.arange(0, 100, 10))

    #ax2.set_title('Temperature sensor',fontdict={'color':'blue','weight':'bold','size' : 13}, pad=14)
    ax2.set_xlabel('Time[s]',fontdict={'color':'black','weight':'bold','size' : 9})
    ax2.set_ylabel('Temperature[°C]',fontdict={'color':'black','weight':'bold','size' : 13})
    
    ax2.tick_params(axis='both', which='major', labelsize=13)
    ax2.tick_params(axis='both', which='minor', labelsize=11)
    ax2.set_yticks(np.arange(-2, 50, 5))
    ax2.set_xticks(np.arange(0, 100, 10))

def humidity_sensor(event):
    global channel
    channel = 0
    
    file = open(DEVICE_FILE, "w")
    file.write("0")
    file.close()
    

def temperature_sensor(event):
    global channel
    channel = 1
    
    file = open(DEVICE_FILE, "w")
    file.write("1")
    file.close()
    

def T_H_sensor(event):
    global channel
    channel = 2

    file = open(DEVICE_FILE, "w")
    file.write("2")
    file.close()
    

axbutton1 = plt.axes([0.27, 0.90, 0.05, 0.05]) #left bottom width heigth
axbutton2 = plt.axes([0.7, 0.90, 0.05, 0.05])
axbutton3 = plt.axes([0.46, 0.92, 0.05, 0.05])

btn1 = Button(ax=axbutton1, label='H', color='white', hovercolor='green')
btn2 = Button(ax=axbutton2, label='T', color='white', hovercolor='green')
btn3 = Button(ax=axbutton3, label='T & H', color='white', hovercolor='green')

btn1.on_clicked(humidity_sensor)
btn2.on_clicked(temperature_sensor)
btn3.on_clicked(T_H_sensor)

set_figure()

animacion = animation.FuncAnimation(fig, plotter, interval=1000)

plt.show()