import math

f = open('time.txt', 'r')
data = f.readlines()
time = 0
avg = 0

for x in data:
    fpn = float(x)
    time += fpn
    if (avg == 0):
        avg += fpn
    else:
        avg = (avg + fpn) / 2

print(time)
print(avg)

