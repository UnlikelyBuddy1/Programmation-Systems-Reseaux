
with open("test.txt", "w") as logs:
    for i in range(0,500):
        logs.write("{} Python is a great programing language, it works with an interpreter an is easy to use. The disadvantage is that Python is slow, slow like a snake\n".format(i))