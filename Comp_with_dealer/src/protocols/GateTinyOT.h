// By Kaiwen Wang
/// @file Base class for gates using TinyOT shares

#ifndef MD_ML_PROTOCOLS_GATETINYOT_H
#define MD_ML_PROTOCOLS_GATETINYOT_H

#include <vector>
#include <memory>

#include "networking/Party.h"
#include "share/IsTinyOTShare.h"
#include "protocols/PartyWithFakeOfflineTinyOT.h"


namespace md_ml {

template <IsTinyOTShare ShrType>
class CircuitTinyOT;

template <IsTinyOTShare ShrType>
class GateTinyOT {

    friend class CircuitTinyOT<ShrType>;
public:
    using ClearType = typename ShrType::ClearType;         // bool
    using SemiShrType = typename ShrType::SemiShrType;     // bool
    using MacType = typename ShrType::MacType;             // F_{2^S}

    // 添加单输入构造函数
    GateTinyOT(const std::shared_ptr<GateTinyOT>& p_input_x,
               PartyWithFakeOfflineTinyOT<ShrType>& p_party)
        : party_(p_party), input_x_(p_input_x), input_y_(nullptr) {}

    GateTinyOT(PartyWithFakeOfflineTinyOT<ShrType>& p_party,
               std::size_t p_dim_row, std::size_t p_dim_col);

    GateTinyOT(const std::shared_ptr<GateTinyOT>& p_input_x,
               const std::shared_ptr<GateTinyOT>& p_input_y);
    
    // 新增：多输入构造函数
    GateTinyOT(const std::vector<std::shared_ptr<GateTinyOT>>& p_inputs);

    GateTinyOT(const std::vector<std::shared_ptr<GateTinyOT>>& p_inputs, 
                PartyWithFakeOfflineTinyOT<ShrType>& p_party);

    virtual ~GateTinyOT() = default;

    void RunOffline();
    void readOfflineFromFile();
    void RunOnline();

    bool isEvaluatedOnline() const { return evaluated_online_; }
    void setEvaluatedOnline(bool value) { evaluated_online_ = value; }

    [[nodiscard]] auto& party() { return party_; }

    [[nodiscard]] std::size_t my_id() const { return party_.my_id(); }

    [[nodiscard]] std::size_t dim_row() const { return dim_row_; }
    [[nodiscard]] std::size_t dim_col() const { return dim_col_; }

    [[nodiscard]] auto input_x() { return input_x_; }
    [[nodiscard]] auto input_y() { return input_y_; }

    [[nodiscard]] auto read_offline() {return read_offline_; }
    [[nodiscard]] auto evaluated_online() {return evaluated_online_; }

    // 新增：多输入访问器
    [[nodiscard]] const auto& inputs() const { return inputs_; }

    // 掩码的比特份额
    [[nodiscard]] const std::vector<SemiShrType>& lambda_shr() const { return lambda_shr_; }
    [[nodiscard]] std::vector<SemiShrType>& lambda_shr() { return lambda_shr_; }

    // 掩码的MAC份额
    [[nodiscard]] const std::vector<MacType>& lambda_shr_mac() const { return lambda_shr_mac_; }
    [[nodiscard]] std::vector<MacType>& lambda_shr_mac() { return lambda_shr_mac_; }

    // 打开的掩码值（δ值）
    [[nodiscard]] const std::vector<SemiShrType>& Delta_clear() const { return Delta_clear_; }
    [[nodiscard]] std::vector<SemiShrType>& Delta_clear() { return Delta_clear_; }

protected:
    void set_dim_row(std::size_t p_dim_row) { dim_row_ = p_dim_row; }
    void set_dim_col(std::size_t p_dim_col) { dim_col_ = p_dim_col; }
    // 输入线
    std::shared_ptr<GateTinyOT> input_x_{};
    std::shared_ptr<GateTinyOT> input_y_{};

    // 新增：多输入线（用于k输入门）
    std::vector<std::shared_ptr<GateTinyOT>> inputs_{};


private:
    virtual void doRunOffline() {
        throw std::runtime_error("Offline Phase is not implemented.");
    }
    virtual void doReadOfflineFromFile() = 0;
    virtual void doRunOnline() = 0;

    bool evaluated_offline_ = false;
    bool evaluated_online_ = false;
    bool read_offline_ = false;

    PartyWithFakeOfflineTinyOT<ShrType>& party_;

    // 门实际上持有一个矩阵，不是单个值
    std::size_t dim_row_ = 1;
    std::size_t dim_col_ = 1;

    std::vector<SemiShrType> lambda_shr_;     // 掩码的比特份额
    std::vector<MacType> lambda_shr_mac_;     // 掩码的MAC份额
    std::vector<SemiShrType> Delta_clear_;    // 打开的掩码值
};


template <IsTinyOTShare ShrType>
GateTinyOT<ShrType>::GateTinyOT(PartyWithFakeOfflineTinyOT<ShrType>& p_party,
                                 std::size_t p_dim_row, std::size_t p_dim_col)
    : party_(p_party), dim_row_(p_dim_row), dim_col_(p_dim_col) {}


template <IsTinyOTShare ShrType>
GateTinyOT<ShrType>::GateTinyOT(const std::shared_ptr<GateTinyOT>& p_input_x,
                                 const std::shared_ptr<GateTinyOT>& p_input_y)
    : party_(p_input_x->party()), input_x_(p_input_x), input_y_(p_input_y) {}

// 新增：多输入构造函数实现
template <IsTinyOTShare ShrType>
GateTinyOT<ShrType>::GateTinyOT(const std::vector<std::shared_ptr<GateTinyOT>>& p_inputs)
    : party_(p_inputs[0]->party()), inputs_(p_inputs) {
    
    if (p_inputs.empty()) {
        throw ::std::runtime_error("GateTinyOT: inputs vector cannot be empty");
    }
}

template <IsTinyOTShare ShrType>
GateTinyOT<ShrType>::GateTinyOT(const std::vector<std::shared_ptr<GateTinyOT>>& p_inputs, 
                                PartyWithFakeOfflineTinyOT<ShrType>& p_party)
    : party_(p_party), inputs_(p_inputs) {
    if (p_inputs.empty()) {
        throw ::std::runtime_error("GateTinyOT: inputs vector cannot be empty");
    }
}

template <IsTinyOTShare ShrType>
void GateTinyOT<ShrType>::RunOffline() {
    if (this->evaluated_offline_)
        return;

    if (input_x_ && !input_x_->evaluated_offline_)
        input_x_->RunOffline();
    if (input_y_ && !input_y_->evaluated_offline_)
        input_y_->RunOffline();

    //  新增：处理多输入门（递归调用）
    if (!inputs_.empty()) {
        for (auto& input : inputs_) {
            if (input && !input->evaluated_offline_) {
                input->RunOffline();
            }
        }
    }
    
    this->doRunOffline();

    this->evaluated_offline_ = true;
}


template <IsTinyOTShare ShrType>
void GateTinyOT<ShrType>::readOfflineFromFile() {
    if (this->read_offline_)
        return;

    if (input_x_ && !input_x_->read_offline_)
        input_x_->readOfflineFromFile();
    if (input_y_ && !input_y_->read_offline_)
        input_y_->readOfflineFromFile();

    // 新增：处理多输入门（递归调用）
    if (!inputs_.empty()) {
        for (auto& input : inputs_) {
            if (input && !input->read_offline_) {
                input->readOfflineFromFile();
            }
        }
    }
    
    this->doReadOfflineFromFile();

    this->read_offline_ = true;
}


template <IsTinyOTShare ShrType>
void GateTinyOT<ShrType>::RunOnline() {
    if (this->evaluated_online_)
        return;

    if (input_x_ && !input_x_->evaluated_online_)
        input_x_->RunOnline();
    if (input_y_ && !input_y_->evaluated_online_)
        input_y_->RunOnline();

    // 新增：处理多输入门（递归调用）
    if (!inputs_.empty()) {
        for (auto& input : inputs_) {
            if (input && !input->evaluated_online_) {
                input->RunOnline();
            }
        }
    }
    
    this->doRunOnline();

    this->evaluated_online_ = true;
}

} // namespace md_ml

#endif //MD_ML_PROTOCOLS_GATETINYOT_H