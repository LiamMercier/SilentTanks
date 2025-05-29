#include "header.h"
#include "command.h"

bool Header::valid()
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
        case HeaderType::SendCommand:
        {
            if (payload_len != Command::COMMAND_SIZE)
            {
                return false;
            }
            break;
        }
        case HeaderType::Text:
        {
            if (!(payload_len >= 1))
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
        default:
        {
            // do nothing, should never happen
            break;
        }
    }

    return true;
}
