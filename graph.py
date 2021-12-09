import matplotlib.pyplot as plt

plt.style.use('fivethirtyeight')

x=[]
with open("log.txt", "r") as logs:
    lines = logs.readlines()

for line in lines:
    line = line.strip()
    line = int(line)
    x.append(line)

plt.ylim([0, 11])
#plt.xlim([0, 50])
plt.plot(x)
plt.tight_layout()
plt.show()