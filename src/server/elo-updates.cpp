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

#include "elo-updates.h"

#include <cmath>

// We are given as input the initial elos and placement for each player,
// indexed by the player ID for this match.
//
// We return the updated elo's also indexed by user ID.
std::vector<int> elo_updates(const std::vector<int> & initial_elos,
                             const std::vector<uint8_t> & placement)
{
    int N = placement.size();

    // Guard against bad calls.
    if (N <= 1)
    {
        return initial_elos;
    }

    std::vector<double> normalized(N);

    // Elimination order is from 0 (last place) to N-1 (first place).
    //
    // Thus, we compute our normalization as:
    //
    // S_i = placement_i / (N - 1)
    for (int i = 0; i < N; i++)
    {
        normalized[i] = static_cast<double>(placement[i]) /
                        static_cast<double>(N - 1);
    }

    // Now, compute the expected score for each player.
    std::vector<double> expected(N);

    // For each player we compute the expected score
    // against the other players.
    //
    // This means doing E_ij = (1 / 1 + 10^((R_j - R_i)/400))
    // for all j != i and summing E_i = 1/(N-1) * sum_{j}(E_ij)
    //
    // Technically, you can save some compute time on this, but
    // for any reasonable N it will not make much difference.
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            if (i == j)
            {
                continue;
            }

            double expected_ij;

            // Compute rating_j - rating_i
            double elo_diff = static_cast<double>
                                    (
                                        initial_elos[j] - initial_elos[i]
                                    );

            // Divide by 400.0 as in the normal elo formulation.
            double elo_diff_normed = elo_diff / static_cast<double>(400.0);

            // We want to protect against numerical instability,
            // so we clamp E_ij at 1 : 10^7 odds since they are
            // essentially the same as being 0.0 or 1.0 anyways.
            if (elo_diff_normed > ELO_DIFF_UPPER)
            {
                expected_ij = static_cast<double>(0.0);
            }
            else if (elo_diff_normed < ELO_DIFF_LOWER)
            {
                expected_ij = static_cast<double>(1.0);
            }
            else
            {
                expected_ij = 1.0 / (1.0 + std::pow(10.0, elo_diff_normed));
            }

            expected[i] += expected_ij;
        }

        // After adding values for each opponent, divide by N - 1 for average.
        expected[i] = expected[i] / static_cast<double>(N - 1);

    }

    // Scale the K factor based on the number of players.
    double k_scaled = static_cast<double>(K_TWO_PLAYERS) * std::log2(N);

    // Now we update elo rating for each player.
    //
    // The formula for this is simply the standard formula with our normalized
    // scores. We take rating_i_new = rating_i + K_scaled * (scored - expected)
    std::vector<int> updated_elos(N);

    for (int i = 0; i < N; i++)
    {
        double delta_R = k_scaled * (normalized[i] - expected[i]);
        updated_elos[i] = static_cast<int>
                                    (
                                        std::round(initial_elos[i] + delta_R)
                                    );

        // Apply elo floor to ensure elos do not dip below a certain threshold.
        //
        // This can result in long term inflation as some bits of elo will
        // eventually flow up the ladder to better players assuming many games.
        if (updated_elos[i] < ELO_FLOOR)
        {
            updated_elos[i] = ELO_FLOOR;
        }
    }

    return updated_elos;

}
