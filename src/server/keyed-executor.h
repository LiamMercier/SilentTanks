// Copyright (c) 2025 Liam Mercier
//
// This file is part of SilentTanks.
//
// SilentTanks is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License Version 3.0
// as published by the Free Software Foundation.
//
// SilentTanks is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
// for more details.
//
// You should have received a copy of the GNU Affero General Public License v3.0
// along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

#pragma once

#include <mutex>
#include <unordered_map>
#include <memory>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/functional/hash.hpp>

namespace asio = boost::asio;

// Automatically handles strand post logic for key value pairs.
template <typename Key, typename Hash = std::hash<Key>>
class KeyedExecutor
{
public:
    explicit KeyedExecutor(asio::any_io_executor ex)
    : executor_(std::move(ex))
    {
    }

    // Since we need to run different functions.
    template <typename Func>
    void post(const Key & k, Func && f)
    {
        std::shared_ptr<Entry> entry;

        // Lock the global mutex.
        std::lock_guard<std::mutex> lock(map_mutex_);

        // Find the strand if it exists.
        auto iter = map_.find(k);
        if (iter == map_.end())
        {
            // Create it otherwise.
            auto strand = std::make_shared<Entry>(k, executor_);
            map_[k] = strand;
            entry = std::move(strand);
        }
        else
        {
            entry = iter->second;
        }

        // Add one more pending handler.
        entry->pending.fetch_add(1, std::memory_order_relaxed);

        // Now post into the strand.
        asio::post(*entry->strand,
            [this, entry, func = std::forward<Func>(f)]() mutable {

            // Do the function work
            func();

            // Decrease the number of pending handlers. Delete this if
            // there is no more pending work.
            if (entry->pending.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                std::lock_guard<std::mutex> global_lock(map_mutex_);
                auto iterator = map_.find(entry->key);

                if (iterator != map_.end()
                    && iterator->second.get() == entry.get())
                {
                       map_.erase(iterator);
                }
            }
        });
    }

private:
    // Bundle the strand, work count, and key in the entry.
    struct Entry
    {
        Key key;
        std::shared_ptr<asio::strand<asio::any_io_executor>> strand;
        std::atomic<size_t> pending{0};

        Entry(const Key & k, asio::any_io_executor ex)
        : key(k),
        strand(std::make_shared<asio::strand<asio::any_io_executor>>(ex))
        {}

    };


private:
    asio::any_io_executor executor_;
    std::mutex map_mutex_;
    std::unordered_map<Key, std::shared_ptr<Entry>, Hash> map_;
};
