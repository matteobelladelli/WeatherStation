import datetime as dt
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import time
import serial

# Create figure for plotting
arduino = serial.Serial('COM3', 9600, timeout=0)

fig, (ax, ax2, ax3, ax4) = plt.subplots(4, 1, figsize=(15, 8))
plt.subplots_adjust(hspace=1)
xs = []
ys1 = []
ys2 = []
ys3 = []
ys4 = []

# This function is called periodically from FuncAnimation
def animate(i, xs, ys1,ys2,ys3,ys4):

    # Read temperature (Celsius) from TMP102
    if arduino.in_waiting == 4:
        y1 = int.from_bytes(arduino.read(1), byteorder='little')
        y2 = int.from_bytes(arduino.read(1), byteorder='little')
        y3 = int.from_bytes(arduino.read(1), byteorder='little')
        y4 = int.from_bytes(arduino.read(1), byteorder='little')
        xs.append(dt.datetime.now().strftime('%H:%M:%S'))
        ys1.append(y1)
        ys2.append(y2)
        ys3.append(y3)
        ys4.append(y4)

    # Add x and y to lists
    xs = xs[-30:]
    ys1 = ys1[-30:]
    ys2 = ys2[-30:]
    ys3 = ys3[-30:]
    ys4 = ys4[-30:]

    ax.clear()
    ax2.clear()
    ax3.clear()
    ax4.clear()

    ax.set_ylim(0, 50)
    ax2.set_ylim(0, 100)
    ax3.set_ylim(0, 40)
    ax4.set_ylim(0, 100)
    ax.plot(xs, ys1, color='r')
    ax2.plot(xs, ys2, color='b')
    ax3.plot(xs, ys3, color='g')
    ax4.plot(xs, ys4, color='y')
    # Format plot

    plt.setp(ax.xaxis.get_majorticklabels(), rotation=90)
    plt.setp(ax2.xaxis.get_majorticklabels(), rotation=90)
    plt.setp(ax3.xaxis.get_majorticklabels(), rotation=90)
    plt.setp(ax4.xaxis.get_majorticklabels(), rotation=90)
    fig.canvas.set_window_title('WeatherStation')
    ax.set_ylabel('TEMP (°C)')
    ax2.set_ylabel('HUM (%)')
    ax3.set_ylabel('WL (mm)')
    ax4.set_ylabel('LIGHT (%)')

# Set up plot to call animate() function periodically
ani = animation.FuncAnimation(fig, animate, fargs=(xs, ys1, ys2 ,ys3 ,ys4))
plt.show()