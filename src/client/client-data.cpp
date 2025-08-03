#include "client-data.h"

void ClientData::load_user_list(UserList user_list,
                                UserListType list_type)
{
    switch (list_type)
    {
        case UserListType::Friends:
        {
            for (const auto & user : user_list.users)
            {
                friends[user.user_id] =  user;
            }
            break;
        }
        case UserListType::FriendRequests:
        {
            for (const auto & user : user_list.users)
            {
                friend_requests[user.user_id] =  user;
            }
            break;
        }
        case UserListType::Blocks:
        {
            for (const auto & user : user_list.users)
            {
                blocked_users[user.user_id] =  user;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}
