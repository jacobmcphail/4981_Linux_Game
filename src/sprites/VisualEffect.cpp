#include "VisualEffect.h"
#include "Renderer.h"
#include <SDL2/SDL.h>
#include <thread>

VisualEffect VisualEffect::sInstance;

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * sets the ids to 0 the rest can be default initialized
 */
VisualEffect::VisualEffect():preLineId(0), preRectId(0), preTexId(0), postLineId(0),
    postRectId(0), postTexId(0) {}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * helper function to calculate the screen rect based on the camera
 */
inline constexpr SDL_Rect relative(const SDL_Rect& dest, const SDL_Rect& camera){
    return {dest.x - camera.x, dest.y - camera.y, dest.w, dest.h};
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * render and remove timed out effects
 */
void VisualEffect::renderPreEntity(const SDL_Rect &camera) {
    auto& renderer = Renderer::instance();
    SDL_Renderer *rend = renderer.getRenderer();
    {
        std::lock_guard<std::mutex> lock(preTexMut);
        for (auto p = preTex.begin(); p != preTex.end();) {
            if (--p->second.dur > 0 && SDL_HasIntersection(&p->second.dest, &camera)) {
                const SDL_Rect temp = relative(p->second.dest, camera);
                renderer.render(temp, p->second.tex, p->second.src, p->second.angle);
                ++p;
            } else {
                p = preTex.erase(p);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(preLineMut);
        for (auto p = preLines.begin(); p != preLines.end();) {
            if (--p->second.dur > 0) {
                SDL_SetRenderDrawColor(rend, p->second.r, p->second.g, p->second.b, p->second.a);
                SDL_RenderDrawLine(rend, p->second.x - camera.x, p->second.y - camera.y,
                       p->second.ex - camera.x, p->second.ey - camera.y);

                SDL_RenderDrawLine(rend, p->second.x - camera.x + 1, p->second.y - camera.y + 1,
                    p->second.ex - camera.x + 1, p->second.ey - camera.y + 1);

                SDL_RenderDrawLine(rend, p->second.x - camera.x - 1, p->second.y - camera.y - 1,
                    p->second.ex - camera.x - 1, p->second.ey - camera.y - 1);
                ++p;
            } else {
                p = preLines.erase(p);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(preRectMut);
        for (auto p = preRects.begin(); p != preRects.end();) {
            if (--p->second.dur > 0) {
                SDL_SetRenderDrawColor(rend, p->second.r, p->second.g, p->second.b, p->second.a);
                const SDL_Rect temp = relative(p->second.s, camera);
                SDL_RenderDrawRect(rend, &temp);
                ++p;
            } else {
                p = preRects.erase(p);
            }
        }
    }
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * render and remove timed out effects
 */
void VisualEffect::renderPostEntity(const SDL_Rect &camera) {
    auto& renderer = Renderer::instance();
    SDL_Renderer *rend = renderer.getRenderer();
    {
        std::lock_guard<std::mutex> lock(postTexMut);
        for (auto p = postTex.begin(); p != postTex.end();) {
            if (--p->second.dur > 0 && SDL_HasIntersection(&p->second.dest, &camera)) {
                const SDL_Rect temp = relative(p->second.dest, camera);
                renderer.render(temp, p->second.tex, p->second.src, p->second.angle);
                ++p;
            } else {
                p = postTex.erase(p);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(postLineMut);
        for (auto p = postLines.begin(); p != postLines.end();) {
            if (--p->second.dur > 0) {
                SDL_SetRenderDrawColor(rend, p->second.r, p->second.g, p->second.b, p->second.a);
                SDL_RenderDrawLine(rend, p->second.x - camera.x, p->second.y - camera.y,
                        p->second.ex - camera.x, p->second.ey - camera.y);
                ++p;
            } else {
                p = postLines.erase(p);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(postRectMut);
        for (auto p = postRects.begin(); p != postRects.end();) {
            if (--p->second.dur > 0) {
                SDL_SetRenderDrawColor(rend, p->second.r, p->second.g, p->second.b, p->second.a);
                const SDL_Rect temp = relative(p->second.s, camera);
                SDL_RenderDrawRect(rend, &temp);
                ++p;
            } else {
                p = postRects.erase(p);
            }
        }
    }
}


/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * add a line effect
 */
int VisualEffect::addPreLine (const int dur, const int startx, const int starty, const int endx,
        const int endy, const Uint8 r, const Uint8 g, const Uint8 b, const Uint8 a) {
    std::lock_guard<std::mutex> lock(preLineMut);
    preLines[++preLineId] = {dur, startx, starty, endx, endy, r, g, b, a};
    return preLineId;
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * add a line effect
 */
int VisualEffect::addPostLine(const int dur, const int startx, const int starty, const int endx,
        const int endy, const Uint8 r, const Uint8 g, const Uint8 b, const Uint8 a) {
    std::lock_guard<std::mutex> lock(postLineMut);
    postLines[++postLineId] = {dur, startx, starty, endx, endy, r, g, b, a};
    return postLineId;
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * add a rect effect
 */
int VisualEffect::addPreRect(const int dur, const SDL_Rect &dest, const Uint8 r,
        const Uint8 g, const Uint8 b, const Uint8 a) {
    std::lock_guard<std::mutex> lock(preRectMut);
    preRects[++preRectId] = {dur, dest, r, g, b, a};
    return preRectId;
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * add a rect effect
 */
int VisualEffect::addPostRect(const int dur, const SDL_Rect &dest, const Uint8 r,
        const Uint8 g, const Uint8 b, const Uint8 a) {
    std::lock_guard<std::mutex> lock(postRectMut);
    postRects[++postRectId] = {dur, dest, r, g, b, a};
    return postRectId;
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * add a texture effect
 */
int VisualEffect::addPreTex(const int dur, const SDL_Rect &src, const SDL_Rect &dest,
        const TEXTURES tex, const double angle) {
    std::lock_guard<std::mutex> lock(preTexMut);
    preTex[++preTexId] = {dur, tex, src, dest, angle};
    return preTexId;
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * add a texture effect
 */
int VisualEffect::addPostTex(const int dur, const SDL_Rect &src, const SDL_Rect &dest,
        const TEXTURES tex, const double angle) {
    std::lock_guard<std::mutex> lock(postTexMut);
    postTex[++postTexId] = {dur, tex, src, dest, angle};
    return postTexId;
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * cancel effect with given id
 */
void VisualEffect::removePreLine(const int id) {
    std::lock_guard<std::mutex> lock(preLineMut);
    preLines.erase(id);
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * cancel effect with given id
 */
void VisualEffect::removePreRect(const int id) {
    std::lock_guard<std::mutex> lock(preRectMut);
    preRects.erase(id);
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * cancel effect with given id
 */
void VisualEffect::removePreTex(const int id) {
    std::lock_guard<std::mutex> lock(preTexMut);
    preTex.erase(id);
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * cancel effect with given id
 */
void VisualEffect::removePostLine(const int id) {
    std::lock_guard<std::mutex> lock(postLineMut);
    postLines.erase(id);
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * cancel effect with given id
 */
void VisualEffect::removePostRect(const int id) {
    std::lock_guard<std::mutex> lock(postRectMut);
    postRects.erase(id);
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: March 25, 2017
 * Notes:
 * cancel effect with given id
 */
void VisualEffect::removePostTex(const int id) {
    std::lock_guard<std::mutex> lock(postTexMut);
    postTex.erase(id);
}


/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: April 8, 2017
 * Notes:
 * add some gore
 */
void VisualEffect::addBlood(const SDL_Rect &dest) {
    std::lock_guard<std::mutex> lock(preTexMut);
    //20 seconds at 60 fps
    static constexpr int BLOOD_LENGTH = 1200;
    static constexpr int BLOOD_IMAGE = 300;
    preTex[++preTexId] = {BLOOD_LENGTH, TEXTURES::BLOOD, {0, 0, BLOOD_IMAGE, BLOOD_IMAGE},
        dest, M_PI * rand()};
}

/**
 * Developer: Isaac Morneau
 * Designer: Isaac Morneau
 * Date: April 8, 2017
 * Notes:
 * add some gore
 */
void VisualEffect::addBody(const SDL_Rect &dest, const int32_t id) {
    std::lock_guard<std::mutex> lock(preTexMut);
    //20 seconds at 60 fps
    static constexpr int BODY_LENGTH = 600;
    static constexpr int BODY_IMAGE_Y = 150;
    static constexpr int BODY_IMAGE_X = 150;
    static constexpr int START_Y = 1025;
    preTex[++preTexId] = {BODY_LENGTH, id % 2 ? TEXTURES::BABY_ZOMBIE : TEXTURES::DIGGER_ZOMBIE, 
        {0, START_Y, BODY_IMAGE_X, BODY_IMAGE_Y},
        {dest.x, dest.y, dest.h, dest.h}};
}
