import os
import re
import xlrd
import xlwt


def get_latency_and_throughput(file_name):
    fd = open(file_name, 'r')
    read_latency = 0
    read_throughput = 0
    write_latency = 0
    write_throughput = 0
    for line in fd:
        m = re.search('\[READ\]\[Latency:(.*?)ns\]\[Throughput:(.*?)MB/s\]', line)
        if not(m is None):
            read_latency = m.group(1)
            read_throughput = m.group(2)
        m = re.search('\[WRITE\]\[Latency:(.*?)ns\]\[Throughput:(.*?)MB/s\]', line)
        if not(m is None):
            write_latency = m.group(1)
            write_throughput = m.group(2)
    fd.close()
    return read_latency, read_throughput, write_latency, write_throughput


m_dir = '2019_11_25'
m_obj = ['mixed']

latency_excel = xlwt.Workbook(encoding='utf-8')
throughput_excel = xlwt.Workbook(encoding='utf-8')

for obj in m_obj:  # 2019/basic
    l_benchmark = os.listdir(m_dir + '/' + obj)
    for benchmark in l_benchmark:  # 2019/mixed/rw
        l_size = os.listdir(m_dir + '/' + obj + '/' + benchmark)
        for size in l_size:  # 2019/mixed/rw/8B
            table_name = benchmark + '_' + size
            read_latency_table = latency_excel.add_sheet('read_' + table_name, cell_overwrite_ok=True)
            read_throughput_table = throughput_excel.add_sheet('read_' + table_name, cell_overwrite_ok=True)
            write_latency_table = latency_excel.add_sheet('write_' + table_name, cell_overwrite_ok=True)
            write_throughput_table = throughput_excel.add_sheet('write_' + table_name, cell_overwrite_ok=True)
            l_thread = os.listdir(m_dir + '/' + obj + '/' + benchmark + '/' + size)
            for thread in l_thread:  # 2019/basic/rw/8B/1_1.result
                m = re.search('(.*?)_(.*?).result', thread)
                num_read_thread = m.group(1)
                num_write_thread = m.group(2)
                name = m_dir + '/' + obj + '/' + benchmark + '/' + size + '/' + thread
                print(name)
                read_latency, read_throughput, write_latency, write_throughput = get_latency_and_throughput(name)
                print(str(read_latency) + '/' + str(read_throughput) + '/' + str(write_latency) + '/' + str(write_throughput))
                read_latency_table.write(int(num_read_thread), int(num_write_thread), read_latency)
                read_throughput_table.write(int(num_read_thread), int(num_write_thread), read_throughput)
                write_latency_table.write(int(num_read_thread), int(num_write_thread), write_latency)
                write_throughput_table.write(int(num_read_thread), int(num_write_thread), write_throughput)

                read_latency_table.write(int(num_read_thread), 0, 'read_' + num_read_thread)
                read_latency_table.write(0, int(num_write_thread), 'write_' + num_write_thread)
                read_throughput_table.write(int(num_read_thread), 0, 'read_' + num_read_thread)
                read_throughput_table.write(0, int(num_write_thread), 'write_' + num_write_thread)
                write_latency_table.write(int(num_read_thread), 0, 'read_' + num_read_thread)
                write_latency_table.write(0, int(num_write_thread), 'write_' + num_write_thread)
                write_throughput_table.write(int(num_read_thread), 0, 'read_' + num_read_thread)
                write_throughput_table.write(0, int(num_write_thread), 'write_' + num_write_thread)


latency_excel.save(m_dir + '_mixed_latency.xls')
throughput_excel.save(m_dir + '_mixed_throughput.xls')
