#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace Nitride
{
// Single message-thread producer, single render-thread consumer. Host MIDI never
// enters this queue. Epoch resets may originate on either thread; indices do not.
class EditorMidiBridge
{
public:
    struct Command {std::uint64_t epoch=0;std::array<std::uint8_t,3> bytes{};};
    static constexpr std::uint32_t capacity=1024,blockBudget=256;
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    bool push(std::uint8_t status,std::uint8_t first,std::uint8_t second) noexcept
    {
        const auto w=write.load(std::memory_order_relaxed);
        if(w-read.load(std::memory_order_acquire)>=capacity){overflows.fetch_add(1);reset();return false;}
        commands[w&(capacity-1)]={generation.load(std::memory_order_acquire),{status,first,second}};
        write.store(w+1,std::memory_order_release);return true;
    }
    std::uint32_t drain(std::array<Command,blockBudget>& destination) noexcept
    {
        const auto r=read.load(std::memory_order_relaxed),w=write.load(std::memory_order_acquire);
        const auto count=std::min(blockBudget,w-r);
        for(std::uint32_t i=0;i<count;++i)destination[i]=commands[(r+i)&(capacity-1)];
        read.store(r+count,std::memory_order_release);return count;
    }
    void reset() noexcept {generation.fetch_add(1,std::memory_order_acq_rel);}
    std::uint64_t epoch() const noexcept {return generation.load(std::memory_order_acquire);}
    std::atomic<std::uint64_t> overflows{0};
    std::array<std::atomic<std::uint16_t>,128> hostHeld{},hostSounding{};
private:
    std::array<Command,capacity> commands{};
    alignas(64) std::atomic<std::uint32_t> write{0};
    alignas(64) std::atomic<std::uint32_t> read{0};
    std::atomic<std::uint64_t> generation{1};
};
}
