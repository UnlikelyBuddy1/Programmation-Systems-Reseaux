import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits import mplot3d

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

with open("logs.csv", "r") as logs:
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

def findMax(array):
    max=0
    for index in range(0, len(array)):
        if array[max] < array[index]:
            max=index
    return max

maxIndex= findMax(bandwidth)
print(cwnd[maxIndex])
print(rto[maxIndex])

fig, ax = plt.subplots()
ax = plt.axes(projection='3d')
ax.scatter3D(cwnd, rto, bandwidth, 'red')
plt.show()