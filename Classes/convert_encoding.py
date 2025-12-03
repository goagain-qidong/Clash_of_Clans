import os
import codecs

# 配置：需要转换的文件后缀
TARGET_EXTENSIONS = ['.cpp', '.h', '.c', '.hpp', '.mm']

def convert_file_to_utf8_bom(file_path):
    # 1. 读取原始文件数据（二进制模式）
    with open(file_path, 'rb') as f:
        raw_data = f.read()

    # 2. 检测并解码
    content = None
    encoding_used = None

    # 尝试检测是不是已经是带 BOM 的 UTF-8
    if raw_data.startswith(codecs.BOM_UTF8):
        # 已经是 UTF-8 with BOM，不需要处理，直接跳过
        # print(f"[跳过] 已经是 UTF-8 BOM: {file_path}")
        return

    # 尝试用 UTF-8 解码 (针对已经是 UTF-8 但没有 BOM 的文件)
    try:
        content = raw_data.decode('utf-8')
        encoding_used = 'utf-8'
    except UnicodeDecodeError:
        # 如果 UTF-8 失败，尝试用 GB18030 解码 (GB18030 兼容 GBK 和 GB2312，且支持生僻字)
        try:
            content = raw_data.decode('gb18030')
            encoding_used = 'gb18030(gbk)'
        except UnicodeDecodeError:
            print(f"!!! [错误] 无法识别编码，跳过: {file_path}")
            return

    # 3. 写入为 UTF-8 with BOM
    # 'utf-8-sig' 是 Python 中专门指代 UTF-8 with BOM 的编码名称
    with open(file_path, 'w', encoding='utf-8-sig') as f:
        f.write(content)

    print(f"[成功] {encoding_used} -> UTF-8 BOM: {file_path}")

def traverse_and_convert(root_dir):
    print(f"开始扫描目录: {root_dir} ...")
    count = 0
    for root, dirs, files in os.walk(root_dir):
        # 过滤掉 .git, .vs, build 等不需要处理的文件夹
        if '.git' in root or '.vs' in root or 'build' in root or 'cocos2d' in root:
            continue

        for file in files:
            # 检查后缀名
            ext = os.path.splitext(file)[1].lower()
            if ext in TARGET_EXTENSIONS:
                full_path = os.path.join(root, file)
                convert_file_to_utf8_bom(full_path)
                count += 1
    print(f"处理完成！共扫描 {count} 个文件。")

if __name__ == '__main__':
    # 获取脚本所在目录
    current_dir = os.getcwd()
    traverse_and_convert(current_dir)
    input("按回车键退出...")