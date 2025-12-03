#pragma once

#include "cocos2d.h"

#include <vector>

#include <queue>

#include <unordered_map>

#include <functional>

#include "GridMap.h"



struct GridNode {

    int x;

    int y;

    float g;

    float f;

};



class Pathfinder {

public:

    // Compute A* path from start to goal on the given grid

    static bool findPath(GridMap* grid,

                         const cocos2d::Vec2& startGrid,

                         const cocos2d::Vec2& goalGrid,

                         std::vector<cocos2d::Vec2>& outPath)

    {

        if (!grid) return false;

        auto width = grid->getGridWidth();

        auto height = grid->getGridHeight();

        auto inBounds = [&](int x, int y){ return x>=0 && y>=0 && x<width && y<height; };

        auto heuristic = [&](int x, int y, int gx, int gy){ return (float)(abs(x-gx) + abs(y-gy)); };



        int sx = (int)startGrid.x, sy = (int)startGrid.y;

        int gx = (int)goalGrid.x, gy = (int)goalGrid.y;

        if (!inBounds(sx, sy) || !inBounds(gx, gy)) return false;

        if (grid->isBlocked(gx, gy)) {

            // Try to find a free neighbor around goal

            const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

            bool found = false;

            for (auto& d : dirs){ int nx=gx+d[0], ny=gy+d[1]; if (inBounds(nx,ny) && !grid->isBlocked(nx,ny)){ gx=nx; gy=ny; found=true; break; } }

            if (!found) return false;

        }



        struct KeyHash { size_t operator()(const std::pair<int,int>& k) const { return ((size_t)k.first<<32) ^ (size_t)k.second; } };

        std::unordered_map<std::pair<int,int>, float, KeyHash> gScore;

        std::unordered_map<std::pair<int,int>, std::pair<int,int>, KeyHash> cameFrom;



        auto cmp = [](const GridNode& a, const GridNode& b){ return a.f > b.f; };

        std::priority_queue<GridNode, std::vector<GridNode>, decltype(cmp)> open(cmp);



        gScore[{sx,sy}] = 0.0f;

        open.push({sx,sy,0.0f,heuristic(sx,sy,gx,gy)});



        const int neighbors[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};



        while(!open.empty()){

            auto cur = open.top(); open.pop();

            if (cur.x == gx && cur.y == gy){

                // reconstruct

                std::vector<cocos2d::Vec2> path;

                std::pair<int,int> key = {gx,gy};

                path.emplace_back((float)gx,(float)gy);

                while(!(key.first == sx && key.second == sy)){

                    auto it = cameFrom.find(key);

                    if (it == cameFrom.end()) break;

                    key = it->second;

                    path.emplace_back((float)key.first, (float)key.second);

                }

                std::reverse(path.begin(), path.end());

                outPath = std::move(path);

                return true;

            }



            for (auto& d : neighbors){

                int nx = cur.x + d[0];

                int ny = cur.y + d[1];

                if (!inBounds(nx,ny) || grid->isBlocked(nx,ny)) continue;

                float step = (abs(d[0]) + abs(d[1]) == 2) ? 1.4f : 1.0f; // diagonal cost

                float tentative = gScore[{cur.x,cur.y}] + step;

                auto nk = std::make_pair(nx,ny);

                auto it = gScore.find(nk);

                if (it == gScore.end() || tentative < it->second){

                    cameFrom[nk] = {cur.x, cur.y};

                    gScore[nk] = tentative;

                    float f = tentative + heuristic(nx,ny,gx,gy);

                    open.push({nx,ny,tentative,f});

                }

            }

        }

        return false;

    }

};