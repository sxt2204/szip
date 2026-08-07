import os
import random

def generate_mixed_file(filename, size_a, size_b, size_c):
    with open(filename, 'wb') as f:
        # A段：极低熵（全 'A'，即 0x41）
        f.write(b'A' * size_a)
        
        # B段：极高熵（纯随机字节/白噪声）
        f.write(os.urandom(size_b))
        
        # C段：极低熵（全 'C'，即 0x43）
        f.write(b'C' * size_c)
        
    print(f"Generated {filename}")
    print(f"  - Part A (Low Entropy) : {size_a} bytes")
    print(f"  - Part B (High Entropy): {size_b} bytes")
    print(f"  - Part C (Low Entropy) : {size_c} bytes")
    print(f"  - Total Size           : {size_a + size_b + size_c} bytes\n")

if __name__ == '__main__':
    # 生成论文里的微型测试样本（用于概念演示）
    generate_mixed_file('test/small_mixed.bin', 100, 10, 200)
    
    # 生成大型基准测试样本（用于实际跑分，比如 10MB + 2MB随机 + 10MB）
    generate_mixed_file('test/large_mixed.bin', 10*1024*1024, 2*1024*1024, 10*1024*1024)
