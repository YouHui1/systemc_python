import mmap
import struct
import ctypes


# 定义结构体的格式
struct_format = "i" * 20  # 假设结构体包含两个int类型的字段
a = [13,23,33,35,63, 10, 11, 12, 13, 14]
b = [11,12,13,15,16, 10, 11, 12, 13, 14]
# 打开保存结构体的文件
# with open("data", "r+b") as file:
#     # 创建mmap对象，映射整个文件
#     mmapped_file = mmap.mmap(file.fileno(), 0)

#     # 读取结构体数据
#     struct_size = struct.calcsize(struct_format)
#     print(struct_size)
#     struct_data = struct.unpack(struct_format, mmapped_file[:struct_size])

#     # 打印结构体字段的值
#     print(struct_data)

#     # 关闭mmap对象
#     mmapped_file.close()


with open("data", "w+b") as file:
    # 创建mmap对象，映射整个文件
    struct_size = struct.calcsize(struct_format)
    print(struct_size)
    file.truncate(struct_size)
    mmapped_file = mmap.mmap(file.fileno(), 0)

    # 读取结构体数据
     # 将C数据类型打包为二进制字符串
    # struct_data = struct.pack(struct_format, 110, b'a', True, 120, *a, 130)  # 示例数据
    struct_data = struct.pack(struct_format, *a, *b)  # 示例数据

    # 将二进制字符串写入mmap对象
    mmapped_file.write(struct_data)

    # 关闭mmap对象
    mmapped_file.close()
