// By Kaiwen Wang

#ifndef MD_ML_PROTOCOLS_GATE_FP_H
#define MD_ML_PROTOCOLS_GATE_FP_H

#include <vector>
#include <memory>
#include <stdexcept>

#include "networking/Party.h"
#include "share/IsFpShare.h"
#include "protocols/PartyWithFakeOfflineFp.h"
#include "share/EmptyType.h"

namespace md_ml {

template <IsFpShare ShrType, bool UseMac = true>
class GateFp {
public:
    using ClearType = typename ShrType::ClearType;
    using SemiShrType = typename ShrType::SemiShrType;
    using MacType = typename ShrType::MacType;

    static constexpr bool kUseMac = UseMac;

    GateFp(PartyWithFakeOfflineFp<ShrType, UseMac>& p_party, std::size_t p_dim_row, std::size_t p_dim_col);
    GateFp(const std::shared_ptr<GateFp>& p_input_x, const std::shared_ptr<GateFp>& p_input_y);
    GateFp(const std::vector<std::shared_ptr<GateFp>>& p_inputs);

    virtual ~GateFp() = default;

    void RunOffline();
    void readOfflineFromFile();
    void RunOnline();

    [[nodiscard]] auto& party() { return party_; }
    [[nodiscard]] std::size_t my_id() const { return party_.my_id(); }
    [[nodiscard]] std::size_t dim_row() const { return dim_row_; }
    [[nodiscard]] std::size_t dim_col() const { return dim_col_; }
    [[nodiscard]] auto input_x() { return input_x_; }
    [[nodiscard]] auto input_y() { return input_y_; }
    [[nodiscard]] const auto& inputs() const { return inputs_; }

    [[nodiscard]] const std::vector<SemiShrType>& lambda_shr() const { return lambda_shr_; }
    [[nodiscard]] std::vector<SemiShrType>& lambda_shr() { return lambda_shr_; }

    // MAC shares 访问器 - 只在 UseMac=true 时可用
    template<bool U = UseMac, typename = std::enable_if_t<U>>
    [[nodiscard]] const std::vector<MacType>& lambda_shr_mac() const { 
        return lambda_shr_mac_; 
    }
    
    template<bool U = UseMac, typename = std::enable_if_t<U>>
    [[nodiscard]] std::vector<MacType>& lambda_shr_mac() { 
        return lambda_shr_mac_; 
    }

    [[nodiscard]] const std::vector<SemiShrType>& Delta_clear() const { return Delta_clear_; }
    [[nodiscard]] std::vector<SemiShrType>& Delta_clear() { return Delta_clear_; }

protected:
    void set_dim_row(std::size_t p_dim_row) { dim_row_ = p_dim_row; }
    void set_dim_col(std::size_t p_dim_col) { dim_col_ = p_dim_col; }

private:
    virtual void doRunOffline() { throw std::runtime_error("Offline Phase is not implemented."); }
    virtual void doReadOfflineFromFile() = 0;
    virtual void doRunOnline() = 0;

    bool evaluated_offline_ = false;
    bool evaluated_online_ = false;
    bool read_offline_ = false;

    PartyWithFakeOfflineFp<ShrType, UseMac>& party_;

    std::shared_ptr<GateFp> input_x_{};
    std::shared_ptr<GateFp> input_y_{};
    std::vector<std::shared_ptr<GateFp>> inputs_{};

    std::size_t dim_row_ = 1;
    std::size_t dim_col_ = 1;

    std::vector<SemiShrType> lambda_shr_;
    
    // 条件成员：只在 UseMac=true 时存在
    std::conditional_t<UseMac, std::vector<MacType>, EmptyType> lambda_shr_mac_;
    
    std::vector<SemiShrType> Delta_clear_;
};


template <IsFpShare ShrType, bool UseMac>
GateFp<ShrType, UseMac>::GateFp(PartyWithFakeOfflineFp<ShrType, UseMac>& p_party, 
                                 std::size_t p_dim_row, std::size_t p_dim_col)
    : party_(p_party), dim_row_(p_dim_row), dim_col_(p_dim_col) {}


template <IsFpShare ShrType, bool UseMac>
GateFp<ShrType, UseMac>::GateFp(const std::shared_ptr<GateFp>& p_input_x, 
                                 const std::shared_ptr<GateFp>& p_input_y)
    : party_(p_input_x->party()), input_x_(p_input_x), input_y_(p_input_y) {}

template <IsFpShare ShrType, bool UseMac>
GateFp<ShrType, UseMac>::GateFp(const std::vector<std::shared_ptr<GateFp>>& p_inputs)
    : party_(p_inputs[0]->party()), inputs_(p_inputs) {
    if (p_inputs.empty()) {
        throw std::runtime_error("GateFp: inputs vector cannot be empty");
    }
}


template <IsFpShare ShrType, bool UseMac>
void GateFp<ShrType, UseMac>::RunOffline() {
    if (this->evaluated_offline_)
        return;

    if (input_x_ && !input_x_->evaluated_offline_)
        input_x_->RunOffline();
    if (input_y_ && !input_y_->evaluated_offline_)
        input_y_->RunOffline();

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


template <IsFpShare ShrType, bool UseMac>
void GateFp<ShrType, UseMac>::readOfflineFromFile() {
    if (this->read_offline_)
        return;

    if (input_x_ && !input_x_->read_offline_)
        input_x_->readOfflineFromFile();
    if (input_y_ && !input_y_->read_offline_)
        input_y_->readOfflineFromFile();

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


template <IsFpShare ShrType, bool UseMac>
void GateFp<ShrType, UseMac>::RunOnline() {
    if (this->evaluated_online_)
        return;

    if (input_x_ && !input_x_->evaluated_online_)
        input_x_->RunOnline();
    if (input_y_ && !input_y_->evaluated_online_)
        input_y_->RunOnline();

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

#endif //MD_ML_PROTOCOLS_GATE_FP_H