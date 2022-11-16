#!/usr/bin/env python
import os
import statistics
import matplotlib.pyplot as plt
import matplotlib.font_manager as font_manager
import matplotlib as mpl


class Result:
    def __init__(self, name, start_node, num_threads, is_top_down, exe_time, MTEPS):
        self.name = name
        self.start_node = start_node
        self.num_threads = num_threads
        self.is_top_down = is_top_down
        self.exe_time = exe_time
        self.MTEPS = MTEPS

    def print_info(self):
        print(f'{self.name}:')
        print(f'  start_node  : {self.start_node}')
        print(f'  num_threads : {self.num_threads}')
        print(f'  is_top_down : {self.is_top_down}')
        print(f'  exe_time    : {self.exe_time}')
        print(f'  MTEPS       : {self.MTEPS}')
        print()


def extract_info(filename):
    lines = []
    info = None
    with open(filename, 'r') as f:
        lines = f.readlines()
    for line in lines:
        if line.startswith("%"):
            continue
        info = line
        break
    assert(info is not None)
    info = info.split(' ')
    assert(len(info) == 2)
    exe_time = float(info[0])
    MTEPS = float(info[1])
    _, name = os.path.split(filename)
    name = name.split('.')[-2]
    name = name.split('-')
    start_node = int(name[-3])
    num_threads = int(name[-2])
    is_top_down = int(name[-1]) == 0
    name = '-'.join(name[:-3])
    result = Result(name, start_node, num_threads,
                    is_top_down, exe_time, MTEPS)
    # result.print_info()
    return result


def main():
    # single threaded
    names = ['web-Stanford', 'roadNet-CA', 'soc-LiveJournal1',
             'com-orkut', 'RMAT1', 'RMAT2', 'RMAT3']
    for name in names:
        result = []
        for i in range(1, 11):
            result.append(extract_info(f"result/{name}-{i}-1-0.txt"))
        exe_times = [i.exe_time for i in result]
        MTEPS = [i.MTEPS for i in result]
        print(f'{name} (Top-Down) & {statistics.mean(exe_times):.4f} & {statistics.mean(MTEPS):.4f} & {statistics.stdev(exe_times):.4f} \\\\')

        result = []
        for i in range(1, 11):
            result.append(extract_info(f"result/{name}-{i}-1-2.txt"))
        exe_times = [i.exe_time for i in result]
        MTEPS = [i.MTEPS for i in result]
        print(f'{name} (HyBrid) & {statistics.mean(exe_times):.4f} & {statistics.mean(MTEPS):.4f} & {statistics.stdev(exe_times):.4f} \\\\')

        print('\\hline')

    fig, (ax1, ax2) = plt.subplots(1, 2)
    # Multi threaded
    names = ['web-Stanford', 'roadNet-CA', 'soc-LiveJournal1', 'com-orkut']
    for name in names:
        MTEPS_threads = []
        for thread in range(1, 9):
            result = []
            for i in range(1, 11):
                result.append(extract_info(
                    f"result/{name}-{i}-{thread}-0.txt"))
            MTEPS = [i.MTEPS for i in result]
            MTEPS_threads.append(statistics.mean(MTEPS))
        print(f'{name} (Top-Down) & {MTEPS_threads[0]:.4f} & {MTEPS_threads[-1]:.4f} & {MTEPS_threads[-1] / MTEPS_threads[0]:.4f} \\\\')
        for idx, i in enumerate(MTEPS_threads[1:]):
            MTEPS_threads[idx + 1] /= MTEPS_threads[0]
        MTEPS_threads[0] = 1
        ax1.plot(list(range(1, 9)), MTEPS_threads,
                 '.-', label=name + ' (Top-Down)')

    ax1.plot(list(range(1, 9)), list(range(1, 9)), '.-', label='linear')
    ax1.set_title('Scalability (Top-Down)')
    ax1.grid()
    ax1.legend()

    for name in names:
        MTEPS_threads = []
        for thread in range(1, 9):
            result = []
            for i in range(1, 11):
                result.append(extract_info(
                    f"result/{name}-{i}-{thread}-2.txt"))
            MTEPS = [i.MTEPS for i in result]
            MTEPS_threads.append(statistics.mean(MTEPS))
        print(f'{name} (Hybrid) & {MTEPS_threads[0]:.4f} & {MTEPS_threads[-1]:.4f} & {MTEPS_threads[-1] / MTEPS_threads[0]:.4f} \\\\')
        for idx, i in enumerate(MTEPS_threads[1:]):
            MTEPS_threads[idx + 1] /= MTEPS_threads[0]
        MTEPS_threads[0] = 1
        ax2.plot(list(range(1, 9)), MTEPS_threads,
                 '.-', label=name + ' (Hybrid)')

    ax2.plot(list(range(1, 9)), list(range(1, 9)), '.-', label='linear')
    ax2.set_title('Scalability (Hybrid)')
    ax2.grid()
    ax2.legend()

    plt.tight_layout()
    plt.savefig('report/figures/scalability.pdf')


if __name__ == '__main__':
    font_dir = ['/usr/share/fonts/OTF/']
    for font in font_manager.findSystemFonts(font_dir):
        print(font)
        font_manager.fontManager.addfont(font)
    mpl.rcParams['font.family'] = 'Latin Modern Roman'
    mpl.rcParams['font.size'] = '12'
    mpl.rcParams['figure.figsize'] = 10, 5
    main()
