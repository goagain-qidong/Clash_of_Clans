import os

# 定义需要修复的文件类型
TARGET_EXTENSIONS = ['.cpp', '.h', '.c', '.hpp', '.mm']

def normalize_newlines(file_path):
    try:
        # 1. 以二进制模式读取，避免 Python 自动转换干扰
        with open(file_path, 'rb') as f:
            content = f.read()

        # 2. 解码 (假设已经是 UTF-8 BOM 了，因为你之前转过)
        # 如果报错，说明不是 UTF-8，尝试 GBK
        try:
            text = content.decode('utf-8-sig')
        except UnicodeDecodeError:
            try:
                text = content.decode('gbk')
            except:
                print(f"[跳过] 无法解码: {file_path}")
                return

        # 3. 核心修复逻辑：统一换行符
        # 先把所有的 \r\n 变成 \n
        text = text.replace('\r\n', '\n')
        # 再把所有的 \r (CR) 变成 \n
        text = text.replace('\r', '\n')
        
        # 4. 重新组合成 Windows 格式 (CRLF)
        # 这一步非常关键：我们手动拼接 \r\n，并以二进制写入，
        # 彻底绕过 Python 和 操作系统的自动转换
        fixed_content = text.replace('\n', '\r\n').encode('utf-8-sig')

        # 5. 写入文件
        with open(file_path, 'wb') as f:
            f.write(fixed_content)
            
        print(f"[已修复] {file_path}")

    except Exception as e:
        print(f"[出错] {file_path}: {e}")

def traverse_and_fix(root_dir):
    print(f"开始修复换行符: {root_dir} ...")
    for root, dirs, files in os.walk(root_dir):
        if '.git' in root or 'build' in root or 'cocos2d' in root:
            continue

        for file in files:
            ext = os.path.splitext(file)[1].lower()
            if ext in TARGET_EXTENSIONS:
                full_path = os.path.join(root, file)
                normalize_newlines(full_path)

if __name__ == '__main__':
    traverse_and_fix(os.getcwd())
    input("按回车键退出...")