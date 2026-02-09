# AND-k门测试实验

这个实验演示了如何使用通用的AND-k门实现任意k输入的AND操作。

## 功能特性

1. **通用性**：支持任意k ≥ 2的AND操作
2. **高效性**：只需1轮通信即可完成计算
3. **批量处理**：支持批量处理多个AND-k操作

## 复杂度说明

- **预处理存储**：O(2^k) - 需要存储所有子集的AND结果
- **在线计算**：O(2^k) - 需要遍历所有子集
- **通信轮次**：O(1) - 只需一次打开操作

## 使用建议

- **小规模**（k ≤ 8）：推荐使用，效率高
- **中等规模**（8 < k ≤ 16）：可以使用，但存储需求较大
- **大规模**（k > 16）：建议使用树形分解策略

## 编译和运行

```bash
# 编译
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# 运行测试
# 1. 生成预处理数据（k=4, 批量大小=1）
./andk_test_fake_offline

# 2. 运行在线计算
./andk_test_party_0  # 在终端1
./andk_test_party_1  # 在终端2
```

## 测试用例

测试4输入AND：
- 输入：[true, true, true, true] -> 输出：true
- 输入：[true, true, true, false] -> 输出：false
- 输入：[false, false, false, false] -> 输出：false