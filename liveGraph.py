import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from mpl_toolkits import mplot3d
import numpy as np

plt.style.use('fivethirtyeight')


perfs={}
bandwidth=[]
nFrags=[]
timeouts=[]
retransmits=[]
totalFrags=[]
inputs=[]
outputs=[]
cwnd=[]
rto=[]

with open("12.csv", "r") as logs:
    lines = logs.readlines()
    for line in lines:
        line=line.replace('\n', '')
        line=line.split('=')
        input=line[0]
        output=line[1]
        input=input.split(':')
        output=output.split(':')
        cwnd.append(int(input[0]))
        rto.append(int(input[1]))
        bandwidth.append(int(output[0]))
        nFrags.append(int(output[1]))
        timeouts.append(int(output[2]))
        retransmits.append(int(output[3]))
        totalFrags.append(int(output[4]))

x_vals = []
y1_vals = []
y2_vals=[]

def findMax(array):
    max=0
    for index in range(0, len(array)):
        if array[max] < array[index]:
            max=index
    return max

def findMin(array):
    min=0
    for index in range(0, len(array)):
        if array[index] < array[min]:
            min=index
    return min

minRTO = findMin(rto)
maxRTO = findMax(rto)
mincwnd= findMin(cwnd)
maxcwnd = findMax(cwnd)
maxIndex= findMax(bandwidth)
print(bandwidth[maxIndex])

fig = plt.figure(figsize=(10, 10), facecolor='black')
ax = plt.subplot(projection='3d')
fig.suptitle('Bandwidth in function of CWND and RTO', fontsize=16)

def animate(i):
    x_vals.append(cwnd[i])
    y1_vals.append(rto[i])
    y2_vals.append(bandwidth[i])
    ax.cla()
    ax.scatter3D(x_vals, y1_vals, y2_vals, label='Bandwidth', s=40*np.ones(len(x_vals)), c=np.abs(x_vals)) # plots every time but doesn't clear axis
    if(i>maxIndex):
        ax.text(x_vals[maxIndex+1], y1_vals[maxIndex+1] + 100, y2_vals[maxIndex+1], "MAX")
    ax.legend(loc='upper left') #avvoid clearing legen
    ax.set_ylim(rto[minRTO], rto[maxRTO])
    ax.set_xlim(cwnd[mincwnd], cwnd[maxcwnd])
    ax.set_xlabel("CWND")
    ax.set_ylabel("RTO (µs)")
    ax.set_xlabel('$CWND$', fontsize=20, rotation=0)
    ax.xaxis.label.set_color('red')
    ax.set_ylabel('$RTO (µs)$', fontsize=20, rotation=0)
    ax.yaxis.label.set_color('green')
    ax.set_zlabel('$Bandwidth$', fontsize=20, rotation=0)
    ax.zaxis.label.set_color('blue')
    if(i==len(bandwidth)-1):
        ax.text(x_vals[maxIndex+1], y1_vals[maxIndex+1] + 100, y2_vals[maxIndex+1], f"        {bandwidth[maxIndex]} with cwnd {cwnd[maxIndex]} and RTO {rto[maxIndex]}")


ani = FuncAnimation(plt.gcf(), animate, interval=1)
plt.show()
