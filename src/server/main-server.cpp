#include <string>
#include <iostream>
#include <thread>

#include "match-instance.h"

int main()
{
    // pretend we are creating a new server thread for a match

    std::string map_file = std::string("envs/test2.txt");
    uint8_t num_players = 2;
    std::vector<PlayerInfo> players{PlayerInfo(0),PlayerInfo(1)};
    //uint64_t initial_time_ms = 60*5*1000;
    uint64_t initial_time_ms = 5000;
    uint64_t increment = 250;

    GameMap map(map_file, 12, 8, 2);

    MatchInstance match(map, players, num_players, initial_time_ms, increment);

    // test preloading commands

    for (uint8_t p = 0; p < num_players; ++p)
    {
        for (uint8_t t = 0; t < map.num_tanks; ++t)
        {
            Command c;
            c.sender = p;
            c.type = CommandType::Place;
            c.payload = static_cast<uint8_t>(2*t + p);
            c.payload_optional = static_cast<uint8_t>(p*3 + t);
            c.sequence_number = t;
            match.receive_command(c);
        }
    }

    // make sure no extra tanks are placed if extra commands are sent
    for (int i = 0; i<4; i++)
    {
        Command c;
        c.sender = 0;
        c.type = CommandType::Place;
        c.payload = static_cast<uint8_t>(4 + i);
        c.payload_optional = static_cast<uint8_t>(4 + i);
        c.sequence_number = 8+i;
        match.receive_command(c);
    }

    // test moving into wall
    for (int i = 0; i<3; i++)
    {
        Command c;
        c.sender = 0;
        c.type = CommandType::Move;
        c.tank_id = 0;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 14+i;
        match.receive_command(c);
    }

    // spawn main game thread
    std::thread server_thread([&]{ match.game_loop(); });

    // rotate tank 2 barrel
    {
        Command c;
        c.sender = 1;
        c.tank_id = 2;
        c.type = CommandType::RotateBarrel;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 19;
        match.receive_command(c);
    }

    // move tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Move;
        c.tank_id = 2;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 20;
        match.receive_command(c);
    }


    // fire tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Fire;
        c.tank_id = 2;
        c.sequence_number = 21;
        match.receive_command(c);
    }

    // player 1 turn
    // test out rotating the barrel or something

    for (int i = 0; i < 3; i++)
    {
        Command c;
        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::RotateBarrel;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 22 + i;
        match.receive_command(c);
    }

    /*
    {
        Command c;
        c.sender = 0;
        c.type = CommandType::Load;
        c.tank_id = 1;
        c.sequence_number = 12
        match.receive_command(c);
    }
    */

    // player 2 turn
    // test out destroying tank 1

    // fire tank 2 - should fail
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Fire;
        c.tank_id = 2;
        c.sequence_number = 28;
        match.receive_command(c);
    }

    // load tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Load;
        c.tank_id = 2;
        c.sequence_number = 29;
        match.receive_command(c);
    }

    // fire tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Fire;
        c.tank_id = 2;
        c.sequence_number = 30;
        match.receive_command(c);
    }

    // load tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Load;
        c.tank_id = 2;
        c.sequence_number = 31;
        match.receive_command(c);
    }

    // player 1 turn

    // player 1 turn
    // test out rotating the barrel or something

    for (int i = 0; i < 3; i++)
    {
        Command c;
        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::RotateBarrel;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 32 + i;
        match.receive_command(c);
    }

    // fire tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Fire;
        c.tank_id = 2;
        c.sequence_number = 33;
        match.receive_command(c);
    }

    // rotate tank 2 twice

    for (int i = 0; i < 2; i++)
    {
        Command c;
        c.sender = 1;
        c.tank_id = 2;
        c.type = CommandType::RotateBarrel;
        c.payload = 1;
        c.payload_optional = 0;
        c.sequence_number = 34 + i;
        match.receive_command(c);
    }

    // player 1 turn

    for (int i = 0; i < 3; i++)
    {
        Command c;
        c.sender = 0;
        c.tank_id = 0;
        c.type = CommandType::RotateBarrel;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 36 + i;
        match.receive_command(c);
    }

    // move tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Move;
        c.tank_id = 2;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 40;
        match.receive_command(c);
    }

    // load tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Load;
        c.tank_id = 2;
        c.sequence_number = 41;
        match.receive_command(c);
    }

    // fire tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Fire;
        c.tank_id = 2;
        c.sequence_number = 42;
        match.receive_command(c);
    }

    // player 1 turn
    for (int i = 0; i < 3; i++)
    {
        Command c;
        c.sender = 0;
        c.tank_id = 0;
        c.type = CommandType::RotateBarrel;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 43 + i;
        match.receive_command(c);
    }

    // load tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Load;
        c.tank_id = 2;
        c.sequence_number = 46;
        match.receive_command(c);
    }

    // fire tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Fire;
        c.tank_id = 2;
        c.sequence_number = 47;
        match.receive_command(c);
    }

    // load tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Load;
        c.tank_id = 2;
        c.sequence_number = 48;
        match.receive_command(c);
    }

    // player 1 turn
    for (int i = 0; i < 3; i++)
    {
        Command c;
        c.sender = 0;
        c.tank_id = 0;
        c.type = CommandType::RotateBarrel;
        c.payload = 0;
        c.payload_optional = 0;
        c.sequence_number = 43 + i;
        match.receive_command(c);
    }

    // fire tank 2
    {
        Command c;
        c.sender = 1;
        c.type = CommandType::Fire;
        c.tank_id = 2;
        c.sequence_number = 52;
        match.receive_command(c);
    }

    // game should end by itself, all tanks killed

    server_thread.join();

    std::cout << "finished";
}
