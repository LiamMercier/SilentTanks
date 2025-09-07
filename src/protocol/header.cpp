#include "header.h"
#include "command.h"
#include <iostream>

bool Header::valid_server()
{
    if (type_ >= HeaderType::MAX_TYPE)
    {
        return false;
    }

    if (payload_len > MAX_PAYLOAD_LEN)
    {
        return false;
    }

    switch(type_)
    {
        case HeaderType::QueueMatch:
        {
            if (payload_len != 1)
            {
                return false;
            }
            break;
        }
        case HeaderType::CancelMatch:
        {
            if (payload_len != 1)
            {
                return false;
            }
            break;
        }
        case HeaderType::FetchMatchHistory:
        {
            if (payload_len != 1)
            {
                return false;
            }
            break;
        }
        case HeaderType::SendCommand:
        {
            if (payload_len != Command::COMMAND_SIZE)
            {
                return false;
            }
            break;
        }
        case HeaderType::DirectTextMessage:
        {
            if (payload_len < 17)
            {
                return false;
            }
            break;
        }
        case HeaderType::ForfeitMatch:
        {
            if (payload_len != 0)
            {
                return false;
            }
            break;
        }
        case HeaderType::SendFriendRequest:
        {
            if (payload_len < 1 || payload_len > MAX_USERNAME_LENGTH)
            {
                return false;
            }
            break;
        }
        case HeaderType::BlockUser:
        {
            if (payload_len < 1 || payload_len > MAX_USERNAME_LENGTH)
            {
                return false;
            }
            break;
        }
        case HeaderType::RespondFriendRequest:
        {
            if (payload_len != 17)
            {
                return false;
            }
            break;
        }
        case HeaderType::RemoveFriend:
        {
            if (payload_len != 16)
            {
                return false;
            }
            break;
        }
        case HeaderType::UnblockUser:
        {
            if (payload_len != 16)
            {
                return false;
            }
            break;
        }
        case HeaderType::MatchReplayRequest:
        {
            if (payload_len != sizeof(uint64_t))
            {
                return false;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return true;
}

bool Header::valid_client()
{
    if (type_ >= HeaderType::MAX_TYPE)
    {
        return false;
    }

    if (payload_len > MAX_CLIENT_PAYLOAD_LEN)
    {
        return false;
    }

    switch(type_)
    {
        case HeaderType::GoodAuth:
        {
            if (payload_len != RANKED_MODES_COUNT * sizeof(uint32_t))
            {
                return false;
            }
            break;
        }
        case HeaderType::DirectTextMessage:
        {
            if (payload_len < 17)
            {
                return false;
            }
            break;
        }
        case HeaderType::BadAuth:
        {
            if (payload_len != 1)
            {
                return false;
            }
            break;
        }
        case HeaderType::BadRegistration:
        {
            if (payload_len != 1)
            {
                return false;
            }
            break;
        }
        case HeaderType::MatchHistory:
        {
            if (payload_len < 1)
            {
                return false;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return true;
}
