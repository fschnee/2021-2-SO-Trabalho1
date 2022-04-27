#!/usr/bin/env python3

import sys

def main(inverted):
    with open(sys.argv[1]) as f: in_data = f.readlines()

    tasks = []
    for line, task in enumerate(in_data):
        sort_priority = line if inverted else len(in_data) - line

        if task.strip() == '': continue

        taskname, tasklen = task.split(' ')
        tasks += [(taskname.strip(), int(tasklen), sort_priority)]

    tasks = list(sorted(tasks, key=lambda v: (v[1], v[2]), reverse=inverted))

    out = [f'Processador_{i + 1}\n' for i in range(int(sys.argv[2]))]
    procs = [0] * int(sys.argv[2])
    curr_iter = 0
    while len(tasks):
        # Tick the processors
        for i in range(len(procs)):
            if procs[i] == 0 and len(tasks):
                taskname, tasklen, _ = tasks.pop()
                out[i] += f'{taskname};{curr_iter};{curr_iter + tasklen}\n'
                procs[i] = tasklen

            procs[i] = procs[i] - 1

        curr_iter += 1

    out = [s.strip() for s in out]
    return '\n\n'.join(out)

if __name__ == '__main__':
    with open('maior_primeiro.txt', 'w') as f: f.write( main(False) )
    with open('menor_primeiro.txt', 'w') as f: f.write( main(True) )
