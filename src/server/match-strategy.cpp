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

#include "match-strategy.h"

CasualTwoPlayerStrategy::CasualTwoPlayerStrategy(asio::io_context & cntx,
                                                 MakeMatchCallback on_match_ready,
                                                 const std::shared_ptr<MapRepository> & map_repo)
: strand_(cntx.get_executor()),
  on_match_ready_(std::move(on_match_ready)),
  map_repo_(map_repo)
{
}

// push the session into the deque
void CasualTwoPlayerStrategy::enqueue(Session::ptr p)
{
    asio::post(strand_,
               [this, p] {

            boost::uuids::uuid user_id = p->get_user_data().user_id;

            // if the lookup does not find this session enqueued already
            if (lookup_.find(user_id) == lookup_.end())
            {
                queue_.push_back(p);
                lookup_.insert(user_id);
                try_form_match();
            }

            });
}

void CasualTwoPlayerStrategy::cancel(Session::ptr p)
{
    asio::post(strand_, [this, p]{

        boost::uuids::uuid user_id = p->get_user_data().user_id;

        if (lookup_.erase(user_id))
        {
            // remove if lookup found a queued player
            //
            // this might be from the middle of the deque
            queue_.erase(std::remove_if(
                            queue_.begin(),
                            queue_.end(),
                            [&](const Session::ptr & s)
                            {
                                return s->get_user_data().user_id == user_id;
                            }),
                            queue_.end());
        }

    });
}

void CasualTwoPlayerStrategy::try_form_match()
{
    while (queue_.size() >= 2)
    {
        // pop player 1
        Session::ptr p1 = queue_.front();
        queue_.pop_front();

        // pop player 2
        Session::ptr p2 = queue_.front();
        queue_.pop_front();

        // erase both from lookups
        lookup_.erase(p1->get_user_data().user_id);
        lookup_.erase(p2->get_user_data().user_id);

        // Get a random map and setup match settings.
        GameMap map = map_repo_->get_random_map
                        (
                            static_cast<uint8_t>(GameMode::ClassicTwoPlayer)
                        );

        MatchSettings settings(map,
                               initial_time_ms,
                               increment_ms,
                               GameMode::ClassicTwoPlayer);

        // spawn a new match instance
        on_match_ready_(std::vector<Session::ptr>{p1, p2}, settings);
    }
}

CasualThreePlayerStrategy::CasualThreePlayerStrategy(
                                        asio::io_context & cntx,
                                        MakeMatchCallback on_match_ready,
                                        const std::shared_ptr<MapRepository> & map_repo)
: strand_(cntx.get_executor()),
  on_match_ready_(std::move(on_match_ready)),
  map_repo_(map_repo)
{
}

// push the session into the deque
void CasualThreePlayerStrategy::enqueue(Session::ptr p)
{
    asio::post(strand_,
               [this, p] {

            boost::uuids::uuid user_id = p->get_user_data().user_id;

            // if the lookup does not find this session enqueued already
            if (lookup_.find(user_id) == lookup_.end())
            {
                queue_.push_back(p);
                lookup_.insert(user_id);
                try_form_match();
            }

            });
}

void CasualThreePlayerStrategy::cancel(Session::ptr p)
{
    asio::post(strand_, [this, p]{

        boost::uuids::uuid user_id = p->get_user_data().user_id;

        if (lookup_.erase(user_id))
        {
            // remove if lookup found a queued player
            //
            // this might be from the middle of the deque
            queue_.erase(std::remove_if(
                            queue_.begin(),
                            queue_.end(),
                            [&](const Session::ptr & s)
                            {
                                return s->get_user_data().user_id == user_id;
                            }),
                            queue_.end());
        }

    });
}

void CasualThreePlayerStrategy::try_form_match()
{
    constexpr uint8_t N_players = players_for_gamemode[
                    static_cast<uint8_t>(GameMode::ClassicThreePlayer)];
    while (queue_.size() >= N_players)
    {
        std::vector<Session::ptr> players;
        players.reserve(N_players);

        for (int i = 0; i < N_players; i++)
        {
            // pop players
            Session::ptr p = queue_.front();
            queue_.pop_front();
            players.push_back(std::move(p));
        }

        // erase them from lookup
        for (const auto & p : players)
        {
            if (p)
            {
                lookup_.erase(p->get_user_data().user_id);
            }
        }

        // Get a random map and setup match settings.
        GameMap map = map_repo_->get_random_map
                        (
                            static_cast<uint8_t>(GameMode::ClassicThreePlayer)
                        );

        MatchSettings settings(map,
                               initial_time_ms,
                               increment_ms,
                               GameMode::ClassicThreePlayer);

        // spawn a new match instance
        on_match_ready_(std::move(players), settings);
    }
}

CasualFivePlayerStrategy::CasualFivePlayerStrategy(
                                        asio::io_context & cntx,
                                        MakeMatchCallback on_match_ready,
                                        const std::shared_ptr<MapRepository> & map_repo)
: strand_(cntx.get_executor()),
  on_match_ready_(std::move(on_match_ready)),
  map_repo_(map_repo)
{
}

// push the session into the deque
void CasualFivePlayerStrategy::enqueue(Session::ptr p)
{
    asio::post(strand_,
               [this, p] {

            boost::uuids::uuid user_id = p->get_user_data().user_id;

            // if the lookup does not find this session enqueued already
            if (lookup_.find(user_id) == lookup_.end())
            {
                queue_.push_back(p);
                lookup_.insert(user_id);
                try_form_match();
            }

            });
}

void CasualFivePlayerStrategy::cancel(Session::ptr p)
{
    asio::post(strand_, [this, p]{

        boost::uuids::uuid user_id = p->get_user_data().user_id;

        if (lookup_.erase(user_id))
        {
            // remove if lookup found a queued player
            //
            // this might be from the middle of the deque
            queue_.erase(std::remove_if(
                            queue_.begin(),
                            queue_.end(),
                            [&](const Session::ptr & s)
                            {
                                return s->get_user_data().user_id == user_id;
                            }),
                            queue_.end());
        }

    });
}

void CasualFivePlayerStrategy::try_form_match()
{
    constexpr uint8_t N_players = players_for_gamemode[
                static_cast<uint8_t>(GameMode::ClassicFivePlayer)];
    while (queue_.size() >= N_players)
    {
        std::vector<Session::ptr> players;
        players.reserve(N_players);

        for (int i = 0; i < N_players; i++)
        {
            // pop players
            Session::ptr p = queue_.front();
            queue_.pop_front();
            players.push_back(std::move(p));
        }

        // erase them from lookup
        for (const auto & p : players)
        {
            if (p)
            {
                lookup_.erase(p->get_user_data().user_id);
            }
        }

        // Get a random map and setup match settings.
        GameMap map = map_repo_->get_random_map
                        (
                            static_cast<uint8_t>(GameMode::ClassicFivePlayer)
                        );

        MatchSettings settings(map,
                               initial_time_ms,
                               increment_ms,
                               GameMode::ClassicFivePlayer);

        // spawn a new match instance
        on_match_ready_(std::move(players), settings);
    }
}

// Ranked 1 vs 1 strategy. Uses a bucket based matching system.
RankedTwoPlayerStrategy::RankedTwoPlayerStrategy(
                        boost::asio::io_context & cntx,
                        MakeMatchCallback on_match_ready,
                        const std::shared_ptr<MapRepository> & map_repo)
:strand_(cntx.get_executor()),
on_match_ready_(std::move(on_match_ready)),
map_repo_(map_repo),
elo_buckets_(NUM_BUCKETS + 1)
{
}

void RankedTwoPlayerStrategy::enqueue(Session::ptr p)
{
    asio::post(strand_,
        [this, p] {

        auto data = p->get_user_data();

        boost::uuids::uuid user_id = data.user_id;

        // if the lookup does not find this session enqueued already
        if (lookup_.find(user_id) == lookup_.end())
        {
            auto now = std::chrono::steady_clock::now();
            size_t bucket_index = elo_to_bucket_index
                                    (
                                        data.get_elo(curr_gamemode)
                                    );

            // Insert the queue entry into the proper bucket and get
            // the iterator back.
            elo_buckets_[bucket_index].push_back({p, now});
            auto iter = std::prev(elo_buckets_[bucket_index].end());

            // Insert the lookup data.
            lookup_[user_id] = {bucket_index, iter};
        }

    });
}

void RankedTwoPlayerStrategy::cancel(Session::ptr p)
{
    asio::post(strand_, [this, p]{

        boost::uuids::uuid user_id = p->get_user_data().user_id;

        auto lookup_itr = lookup_.find(user_id);

        // If entry exists, remove it.
        if (lookup_itr != lookup_.end())
        {
            // Grab the list iterator and bucket from the lookup for erasing.
            LookupData data = lookup_itr->second;
            elo_buckets_[data.bucket_index].erase(data.iter);

            // Also remove the lookup entry after we're done.
            lookup_.erase(lookup_itr);
        }

    });
}

// Simply call our try_form_match on each tick by the match maker.
void RankedTwoPlayerStrategy::tick()
{
    asio::post(strand_, [this]{

        try_form_match();

    });
}

// Compute the current time, pass through each bucket, and match players
// to their closest valid opponent.
//
// Must be called within a stranded function such as tick().
void RankedTwoPlayerStrategy::try_form_match()
{
    auto now = std::chrono::steady_clock::now();
    // Progress from the highest elo bucket to the lowest, attempting
    // to match players versus one another.
    //
    // We would prefer to give higher elo players the first chance at
    // matching.
    for (int i = elo_buckets_.size() - 1; i >= 0; i--)
    {
        auto & bucket = elo_buckets_[i];

        // If this bucket cannot instantly be matched, see if we
        // can look for neighbors.
        if (bucket.size() < 2)
        {
            // If the bucket is empty, just continue.
            if (bucket.empty())
            {
                continue;
            }

            // Pop the first entry, they have been waiting the
            // longest time in this bucket.
            auto & entry = bucket.front();
            auto wait_time = now - entry.enqueued_at;
            auto increments = wait_time / BUCKET_INCREMENT_TIME;

            // Get size_t from our computation. This should always
            // be rounded down since the difference will be positive.
            //
            // NOTE: This means the window delta is based on the oldest
            // player in the queue, so a new person who queues may be matched
            // with someone with a larger elo than expected. We could
            // consider checking that the windows overlap instead
            // if this turns out to be problematic.
            size_t delta = static_cast<size_t>(increments);

            // Enforce delta bounds
            if (delta > MAX_BUCKETS_DIFF)
            {
                delta = MAX_BUCKETS_DIFF;
            }

            // If player has not been queued for enough time, skip.
            if (delta == 0)
            {
                continue;
            }

            // Now search +/- radius buckets for a match.
            for (size_t radius = 1; radius <= delta; radius++)
            {
                // First, lower bucket candidate.
                if (i >= static_cast<int>(radius)
                    && !elo_buckets_[i - radius].empty())
                {
                    match_and_remove(bucket.begin()->session,
                                     elo_buckets_[i - radius].begin()->session);
                    break;
                }

                // Now, upper bucket candidate.
                if (i + static_cast<int>(radius) <= NUM_BUCKETS
                    && !elo_buckets_[i + radius].empty())
                {
                    match_and_remove(bucket.begin()->session,
                                     elo_buckets_[i + radius].begin()->session);
                    break;
                }
            }

            // If no match was found, just continue to the next bucket.
            // We will need to have this player wait.
            continue;
        }
        // Base case, there is at least 2 players in the bucket.
        else
        {
            // Match players from the bucket until there are no more pairs.
            while (bucket.size() >= 2)
            {
                match_and_remove(bucket.begin()->session,
                                 std::next(bucket.begin())->session);
            }
        }
    }
}

void RankedTwoPlayerStrategy::match_and_remove(Session::ptr p1,
                                               Session::ptr p2)
{
    auto p1_uuid = p1->get_user_data().user_id;
    auto p2_uuid = p2->get_user_data().user_id;

    // Remove from buckets and lookup.
    auto p1_look = lookup_.find(p1_uuid);
    auto p2_look = lookup_.find(p2_uuid);

    // Erase p2 first because of iterator order. p2 is std::next of p1.
    elo_buckets_[p2_look->second.bucket_index].erase(p2_look->second.iter);
    elo_buckets_[p1_look->second.bucket_index].erase(p1_look->second.iter);

    lookup_.erase(p1_look);
    lookup_.erase(p2_look);

    // Get a random map and setup match settings.
    GameMap map = map_repo_->get_random_map
                    (
                        static_cast<uint8_t>(GameMode::RankedTwoPlayer)
                    );

    MatchSettings settings(map,
                           initial_time_ms,
                           increment_ms,
                           GameMode::RankedTwoPlayer);

    on_match_ready_(std::vector<Session::ptr>{p1, p2}, settings);

}

size_t RankedTwoPlayerStrategy::elo_to_bucket_index(int elo)
{
    // If greater than our max bucket, put into overflow bucket.
    if (elo >= MAX_ELO_BUCKET)
    {
        return NUM_BUCKETS;
    }
    else if (elo < ELO_FLOOR)
    {
        return 0;
    }
    else
    {
        // This implicitly takes the floor.
        return (elo - ELO_FLOOR) / BUCKET_SIZE;
    }
}
