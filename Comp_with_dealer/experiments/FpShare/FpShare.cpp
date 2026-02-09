// 测试 FpShare.h 实现
#include "share/FpShare.h"
#include "share/IsFpShare.h"
#include <iostream>
#include <cassert>
#include <type_traits>

using namespace md_ml;

// 测试基本属性
void test_basic_properties() {
    std::cout << "=== 测试基本属性 ===" << std::endl;
    
    // 测试 32 位份额
    {
        using Share = FpShare_4294967291;  // 2^32 - 5
        
        std::cout << "FpShare32 (p = 2^32 - 5):" << std::endl;
        std::cout << "  模数 p = " << Share::modulus() << std::endl;
        std::cout << "  明文比特数 kBits = " << Share::kBits << std::endl;
        std::cout << "  MAC安全参数 sBits = " << Share::sBits << std::endl;
        
        // 验证类型
        static_assert(std::is_same_v<Share::ClearType, Share::SemiShrType>);
        static_assert(std::is_same_v<Share::MacType, Share::ClearType>);
        static_assert(std::is_same_v<Share::GlobalKeyType, Share::ClearType>);
        
        // 验证 concept
        static_assert(IsFpShare<Share>);
        static_assert(IsGFPType<Share::ClearType>);
        
        std::cout << "  类型检查: ✓" << std::endl;
    }
    
    // 测试 61 位份额
    {
        using Share = FpShare64;  // 2^61 - 1
        
        std::cout << "\nFpShare64 (p = 2^64 - 59):" << std::endl;
        std::cout << "  模数 p = " << Share::modulus() << std::endl;
        std::cout << "  明文比特数 kBits = " << Share::kBits << std::endl;
        std::cout << "  MAC安全参数 sBits = " << Share::sBits << std::endl;
        
        static_assert(IsFpShare<Share>);
        static_assert(IsGFPType<Share::ClearType>);
        
        std::cout << "  类型检查: ✓" << std::endl;
    }
    
    std::cout << std::endl;
}

// 测试秘密共享模拟
void test_secret_sharing_simulation() {
    std::cout << "=== 模拟秘密共享协议 ===" << std::endl;
    
    using Share = FpShare_4294967291;
    using GF = Share::ClearType;
    
    // 模拟秘密共享过程
    
    // 1. 秘密值
    GF x(12345);
    std::cout << "秘密值 x = " << x << std::endl;
    
    // 2. 生成随机掩码 σx
    GF sigma_x(98765);  // 在实际协议中这应该是随机生成的
    std::cout << "掩码 σx = " << sigma_x << std::endl;
    
    // 3. 计算打开的掩码值 δx = x + σx (mod p)
    GF delta_x = x + sigma_x;
    std::cout << "打开值 δx = x + σx = " << delta_x << std::endl;
    
    // 4. 模拟两方的掩码份额（加性秘密共享）
    // [σx] = σx_1 + σx_2 (mod p)，其中 σx = σx_1 + σx_2
    GF sigma_x_1(45678);  // P1 的份额
    GF sigma_x_2 = sigma_x - sigma_x_1;  // P2 的份额
    std::cout << "P1的掩码份额 [σx]_1 = " << sigma_x_1 << std::endl;
    std::cout << "P2的掩码份额 [σx]_2 = " << sigma_x_2 << std::endl;
    
    // 验证：σx_1 + σx_2 = σx
    GF reconstructed_sigma = sigma_x_1 + sigma_x_2;
    std::cout << "重构掩码: [σx]_1 + [σx]_2 = " << reconstructed_sigma << std::endl;
    assert(reconstructed_sigma == sigma_x);
    std::cout << "掩码重构验证: ✓" << std::endl;
    
    // 5. 模拟 MAC 份额
    // 全局密钥 α (只有一方知道，或分布式生成)
    GF alpha(777777);
    std::cout << "\n全局MAC密钥 α = " << alpha << std::endl;
    
    // MAC值: MAC(σx) = α · σx
    GF mac_sigma_x = alpha * sigma_x;
    std::cout << "MAC值 MAC(σx) = α · σx = " << mac_sigma_x << std::endl;
    
    // MAC份额（加性秘密共享）
    // [MAC·σx] = mac_1 + mac_2 (mod p)
    GF mac_1(111111);  // P1 的 MAC 份额
    GF mac_2 = mac_sigma_x - mac_1;  // P2 的 MAC 份额
    std::cout << "P1的MAC份额 [MAC·σx]_1 = " << mac_1 << std::endl;
    std::cout << "P2的MAC份额 [MAC·σx]_2 = " << mac_2 << std::endl;
    
    // 验证：mac_1 + mac_2 = MAC(σx)
    GF reconstructed_mac = mac_1 + mac_2;
    std::cout << "重构MAC: [MAC·σx]_1 + [MAC·σx]_2 = " << reconstructed_mac << std::endl;
    assert(reconstructed_mac == mac_sigma_x);
    std::cout << "MAC重构验证: ✓" << std::endl;
    
    // 6. 从打开值和掩码份额重构秘密
    // x = δx - σx (mod p)
    GF reconstructed_x = delta_x - sigma_x;
    std::cout << "\n重构秘密: x = δx - σx = " << reconstructed_x << std::endl;
    assert(reconstructed_x == x);
    std::cout << "秘密重构验证: ✓" << std::endl;
    
    std::cout << std::endl;
}

// 测试算术操作
void test_arithmetic_operations() {
    std::cout << "=== 测试份额上的算术操作 ===" << std::endl;
    
    using Share = FpShare64;  // 使用 64 位素数
    using GF = Share::ClearType;
    
    // 模拟两个秘密份额的加法
    std::cout << "【加法操作】" << std::endl;
    GF x(1000);
    GF y(2000);
    std::cout << "x = " << x << ", y = " << y << std::endl;
    
    GF sigma_x(500);
    GF sigma_y(600);
    
    GF delta_x = x + sigma_x;
    GF delta_y = y + sigma_y;
    std::cout << "δx = " << delta_x << ", δy = " << delta_y << std::endl;
    
    // 本地计算：δz = δx + δy
    GF delta_z = delta_x + delta_y;
    std::cout << "δz = δx + δy = " << delta_z << std::endl;
    
    // 验证：z = x + y
    GF sigma_z = sigma_x + sigma_y;
    GF z = delta_z - sigma_z;
    std::cout << "z = δz - σz = " << z << std::endl;
    assert(z == (x + y));
    std::cout << "加法验证: ✓" << std::endl;
    
    // 模拟常量乘法
    std::cout << "\n【常量乘法操作】" << std::endl;
    GF c(3);  // 公开常量
    std::cout << "常量 c = " << c << std::endl;
    
    // 本地计算：δz = c · δx
    delta_z = c * delta_x;
    std::cout << "δz = c · δx = " << delta_z << std::endl;
    
    // 验证：z = c · x
    sigma_z = c * sigma_x;
    z = delta_z - sigma_z;
    std::cout << "z = δz - σz = " << z << std::endl;
    assert(z == (c * x));
    std::cout << "常量乘法验证: ✓" << std::endl;
    
    std::cout << std::endl;
}

// 测试不同素数大小
void test_different_primes() {
    std::cout << "=== 测试不同素数配置 ===" << std::endl;
    
    // 小素数
    {
        using Share = FpShare_1000000007;
        std::cout << "FpShare_1000000007 (10^9 + 7):" << std::endl;
        std::cout << "  p = " << Share::modulus() << std::endl;
        std::cout << "  比特数 = " << Share::kBits << std::endl;
        static_assert(IsFpShare<Share>);
        
        using GF = Share::ClearType;
        GF a(999999999);
        GF b(999999999);
        GF c = a + b;
        std::cout << "  测试: 999999999 + 999999999 = " << c << std::endl;
    }
    
    // 中等素数
    {
        using Share = FpShare31;
        std::cout << "\nFpShare32 (2^31 - 1):" << std::endl;
        std::cout << "  p = " << Share::modulus() << std::endl;
        std::cout << "  比特数 = " << Share::kBits << std::endl;
        static_assert(IsFpShare<Share>);
        
        using GF = Share::ClearType;
        GF a(1000000000);
        GF b(1000000000);
        GF c = a * b;
        std::cout << "  测试: 10^9 * 10^9 = " << c << std::endl;
    }
    
    // 大素数
    {
        using Share = FpShare64;
        std::cout << "\nFpShare64 (2^64 - 59):" << std::endl;
        std::cout << "  p = " << Share::modulus() << std::endl;
        std::cout << "  比特数 = " << Share::kBits << std::endl;
        static_assert(IsFpShare<Share>);
        
        using GF = Share::ClearType;
        GF a(1000000000000000ULL);
        GF b(1000000000000000ULL);
        GF c = a * b;
        std::cout << "  测试: 10^15 * 10^15 = " << c << std::endl;
    }
    
    std::cout << std::endl;
}

// 测试 RemoveUpperBits
void test_remove_upper_bits() {
    std::cout << "=== 测试 RemoveUpperBits ===" << std::endl;
    
    using Share = FpShare_4294967291;
    using GF = Share::ClearType;
    
    GF value(12345);
    GF result = Share::RemoveUpperBits(value);
    
    std::cout << "输入: " << value << std::endl;
    std::cout << "输出: " << result << std::endl;
    assert(result == value);
    std::cout << "RemoveUpperBits验证: ✓" << std::endl;
    
    // 测试向量版本
    std::vector<GF> values = {GF(1), GF(2), GF(3), GF(4), GF(5)};
    auto results = Share::RemoveUpperBits(values);
    assert(results.size() == values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        assert(results[i] == values[i]);
    }
    std::cout << "向量版本验证: ✓" << std::endl;
    
    std::cout << std::endl;
}

int main() {
    try {
        test_basic_properties();
        test_secret_sharing_simulation();
        test_arithmetic_operations();
        test_different_primes();
        test_remove_upper_bits();
        
        std::cout << "=== 所有测试通过！===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}