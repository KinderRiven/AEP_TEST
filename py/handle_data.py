import os
import re
import xlrd
import xlwt


def get_latency_and_throughput(file_name):
    fd = open(file_name, 'r')
    latency = 0
    throughput = 0
    for line in fd:
        m = re.search('\[SUM\]\[Latency:(.*?)ns\]\[Throughput:(.*?)MB/s\]', line)
        if not(m is None):
            latency = m.group(1)
            throughput = m.group(2)
    fd.close()
    return latency, throughput


m_dir = '2019_11_22'
m_obj = ['basic', 'align', 'flush', 'async', 'numa']

latency_excel = xlwt.Workbook(encoding='utf-8')
throughput_excel = xlwt.Workbook(encoding='utf-8')

for obj in m_obj:  # 2019/basic
    l_benchmark = os.listdir(m_dir + '/' + obj)
    for benchmark in l_benchmark:  # 2019/basic/rw
        latency_table = latency_excel.add_sheet(obj + '_' + benchmark, cell_overwrite_ok=True)
        throughput_table = throughput_excel.add_sheet(obj + '_' + benchmark, cell_overwrite_ok=True)
        l_size = os.listdir(m_dir + '/' + obj + '/' + benchmark)
        a = 1
        for size in l_size:  # 2019/basic/rw/8B
            latency_table.write(0, a, size)
            throughput_table.write(0, a, size)
            l_thread = os.listdir(m_dir + '/' + obj + '/' + benchmark + '/' + size)
            b = 1
            for thread in l_thread:  # 2019/basic/rw/8B/thread_1.result
                latency_table.write(b, 0, thread)
                throughput_table.write(b, 0, thread)
                name = m_dir + '/' + obj + '/' + benchmark + '/' + size + '/' + thread
                print(name)
                latency, throughput = get_latency_and_throughput(name)
                # print(latency + '/' + throughput)
                latency_table.write(b, a, latency)
                throughput_table.write(b, a, throughput)
                b += 1
            a += 1

latency_excel.save(m_dir + '_latency.xls')
throughput_excel.save(m_dir + '_throughput.xls')
