import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time
import serial

arduino = serial.Serial('COM3', 9600, timeout=0)

def data_gen():

    t = data_gen.t
    cnt = 0
	
    while True:
        if arduino.in_waiting == 4 :

            y1 = int.from_bytes(arduino.read(1), byteorder='little')
            y2 = int.from_bytes(arduino.read(1), byteorder='little')
            y3 = int.from_bytes(arduino.read(1), byteorder='little')
            y4 = int.from_bytes(arduino.read(1), byteorder='little')
			
            # adapted the data generator to yield both sin and cos
            yield t, y1, y2, y3, y4
            t += 2
            cnt += 1


data_gen.t = 0

# create a figure with two subplots
fig, (ax1,ax2,ax3,ax4) = plt.subplots(4,1,figsize=(10,8))
fig.canvas.set_window_title('WeatherStation')
ax1.set_ylabel('TEMP (°C)')
ax2.set_ylabel('HUM (%)')
ax3.set_ylabel('WL (mm)')
ax4.set_ylabel('LIGHT (%)')

# intialize two line objects (one in each axes)
line1, = ax1.plot([], [], lw=2, color='y')
line2, = ax2.plot([], [], lw=2, color='r')
line3, = ax3.plot([], [], lw=2, color='b')
line4, = ax4.plot([], [], lw=2, color='g')
line = [line1, line2, line3, line4]

# the same axes initalizations as before (just now we do it for both of them)
ax1.set_ylim(0,50)
ax1.set_xlim(0,10,auto='true')
ax1.grid()
ax2.set_ylim(0,100)
ax2.set_xlim(0,10,auto='true')
ax2.grid()
ax3.set_ylim(0,40)
ax3.set_xlim(0,10,auto='true')
ax3.grid()
ax4.set_ylim(0,100)
ax4.set_xlim(0,10,auto='true')
ax4.grid()
	
# initialize the data arrays
xdata, y1data, y2data, y3data, y4data = [], [], [], [], []

def run(data):
    
    # update the data
    t, y1, y2, y3, y4 = data
    xdata.append(t)
    y1data.append(y1)
    y2data.append(y2)
    y3data.append(y3)
    y4data.append(y4)

    # axis limits checking. same as before, just for both axes
    for ax in [ax1,ax2,ax3,ax4]:
        xmin, xmax = ax.get_xlim()
        if t >= xmax:
            ax.set_xlim(xmin, 2*xmax)
            ax.figure.canvas.draw()

    # update the data of both line objects
    line[0].set_data(xdata, y1data)
    line[1].set_data(xdata, y2data)
    line[2].set_data(xdata, y3data)
    line[3].set_data(xdata, y4data)

    return line

ani = animation.FuncAnimation(fig, run, data_gen, interval=2, blit=True, repeat=False)
plt.show()