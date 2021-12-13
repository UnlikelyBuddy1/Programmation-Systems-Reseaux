import subprocess
tests=50
for n in range(0,tests):
    subprocess.run(["./client1", "0", "5000", "test1.txt"])


