import matplotlib.pyplot as plt

'''
best_1 = {}
for num in range(1, 6):
    #times = []
    sum = 0
    with open('peer_' + str(num) + '_times') as f:
        for line in f:
            #times.append(int(line.strip()))
            sum += int(line.strip())
    best_1[num] = sum

for num in best_1:
    print('Worst task 2 time for peer ' +  str(num) + ' is ' + str(best_1[num]))
print('\n')
'''

best_2 = {}
best_2_times = {}
for num in range(0, 6):
    times = []
    sum = 0
    with open('peer_' + str(num) + '_times_task2_best.txt') as f:
        for line in f:
            times.append(int(line.strip()))
            sum += int(line.strip())
    best_2[num] = sum
    best_2_times[num] = times

for num in best_2:
    print('Best 2 time for peer ' +  str(num) + ' is ' + str(best_2[num]))
print('\n')


worst_2 = {}
worst_2_times = {}
for num in range(0, 6):
    times = []
    sum = 0
    with open('peer_' + str(num) + '_times_task2_worst.txt') as f:
        for line in f:
            times.append(int(line.strip()))
            sum += int(line.strip())
    worst_2[num] = sum
    worst_2_times[num] = times

for num in worst_2:
    print('Worst task 2 time for peer ' +  str(num) + ' is ' + str(worst_2[num]))
print('\n')


task1 = {}
task1_times = {}
for num in range(1, 6):
    times = []
    sum = 0
    with open('peer_' + str(num) + '_times_task1.txt') as f:
        for line in f:
            times.append(int(line.strip()))
            sum += int(line.strip())
    task1[num] = sum
    task1_times[num] = times

for num in task1:
    print('Task 1 time for peer ' +  str(num) + ' is ' + str(task1[num]))

def cum_sum(times, size):
    cum_list = []
    cum_list.append(times[0])
    cum_list[0]
    for i in range(1, size):
        try:
            cum_list.append(cum_list[i-1] + times[i])
        except:
            print('index is ' + str(i))
            break
    return cum_list


x = range(1, 51)
plt.plot(x,  cum_sum(task1_times[1], 50))
plt.plot(x,  cum_sum(task1_times[2], 50))
plt.plot(x,  cum_sum(task1_times[3], 50))
plt.plot(x,  cum_sum(task1_times[4], 50))
plt.plot(x,  cum_sum(task1_times[5], 50))
plt.xlabel('# of RFC files')
plt.ylabel('Cumulative time (microseconds)')
plt.title('Task 1 Cumulative Download Time')
plt.legend(['peer 1', 'peer 2', 'peer 3', 'peer 4',  'peer 5'], loc='upper left')
plt.show()