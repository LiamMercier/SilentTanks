BEGIN;

-- Create the users table
--
-- status tracks if the account is in good standing.
CREATE TABLE Users(
    user_id UUID PRIMARY KEY NOT NULL,
    username TEXT UNIQUE NOT NULL,
    hash BYTEA NOT NULL CHECK (octet_length(hash) = 32),
    salt BYTEA NOT NULL CHECK (octet_length(salt) = 16),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_login TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_ip INET NOT NULL
);

-- Enforce case insensitive uniqueness of usernames.
CREATE UNIQUE INDEX idx_users_username_lower ON Users(LOWER(username));

CREATE TABLE Friends(
    user_a UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    user_b UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (user_a, user_b),
    CHECK (user_a < user_b)
);

CREATE INDEX idx_friends_user_b ON Friends(user_b);

CREATE TABLE FriendRequests(
    sender UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    receiver UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    PRIMARY KEY (sender, receiver),
    CHECK (sender <> receiver)
);

CREATE INDEX idx_friend_requests_receiver ON FriendRequests(receiver);

CREATE TABLE BlockedUsers(
    blocker UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    blocked UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (blocker, blocked),
    CHECK (blocker <> blocked)
);

-- Stores ban history for users.
CREATE TABLE UserBans(
    ban_id BIGSERIAL PRIMARY KEY,
    user_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE RESTRICT,
    banned_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    banned_until TIMESTAMPTZ NOT NULL,
    original_expiration TIMESTAMPTZ NOT NULL,
    reason TEXT NOT NULL DEFAULT ''
);

-- And index on the username since we frequently look up users like this.
CREATE INDEX idx_users_username ON Users(username);

-- Create a table to hold IP bans
CREATE TABLE BannedIPs(
    ip INET PRIMARY KEY,
    banned_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    banned_until TIMESTAMPTZ NOT NULL,
    original_expiration TIMESTAMPTZ NOT NULL
);

-- Create the matches table
CREATE TABLE Matches(
    match_id BIGSERIAL PRIMARY KEY,
    game_mode SMALLINT NOT NULL,
    finished_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    settings JSONB NOT NULL,
    move_history JSONB NOT NULL
);

-- Create the match player's table
CREATE TABLE MatchPlayers(
    match_id BIGINT NOT NULL REFERENCES Matches(match_id) ON DELETE CASCADE,
    user_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE RESTRICT,
    player_id SMALLINT NOT NULL,
    placement SMALLINT NOT NULL,
    PRIMARY KEY (match_id, user_id),
    UNIQUE (match_id, player_id)
);

-- Create the elo history table
CREATE TABLE EloHistory(
    history_id BIGSERIAL PRIMARY KEY,
    user_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE RESTRICT,
    match_id BIGINT REFERENCES Matches(match_id) ON DELETE SET NULL,
    game_mode INT NOT NULL,
    old_elo INT NOT NULL,
    new_elo INT NOT NULL
);

-- Create the elo table
CREATE TABLE UserElos(
    user_id UUID NOT NULL REFERENCES Users(user_id),
    game_mode SMALLINT NOT NULL,
    current_elo INT NOT NULL DEFAULT 1000,
    PRIMARY KEY (user_id, game_mode)
);

COMMIT;
