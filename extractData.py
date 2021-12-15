import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits import mplot3d
import matplotlib.pylab as pl

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

with open("cwnd.csv", "r") as logs:
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

def findMin(array):
    min=0
    for index in range(0, len(array)):
        if array[index] < array[min]:
            min=index
    return min

def calculateMean(arrayCwnd, arrayBandwidth):
    meaned=[]
    mean=[]
    actualCwnd=arrayCwnd[0]
    sum=0
    for i in range(0, len(arrayCwnd)):
        if arrayCwnd[i]==actualCwnd:
            mean.append(arrayBandwidth[i])
        else:
            print(arrayCwnd[i])
            sum=0
            for means in mean:
                sum=sum+means
            meaned.append((sum/len(mean)))
            mean=[]
            sum=0
            actualCwnd=arrayCwnd[i]
            mean.append(arrayBandwidth[i])
    return meaned
mean=calculateMean(cwnd, bandwidth)
x_values=np.arange(cwnd[findMin(cwnd)], cwnd[findMax(cwnd)], 2)
print(x_values)
maxIndex= findMax(bandwidth)

fig, ax = plt.subplots()
fig.suptitle('Bandwidth at different CWNDs and Semi-Auto RTO', fontsize=16)
ax.plot(x_values, mean)
plt.show()