/*------------------------------------------------------------------------------
* Source: Weapon.h
*
* Functions:
*
* Date:
*
* Revisions:
* Edited By : Tim Makimov on 2017/APR/10
*
* Designer:
*
* Author:
*
* Notes:
------------------------------------------------------------------------------*/

/*
    Created by Maitiu Morton 2/1/2017
        Edited by DericM 3/8/2017
        Edited by Mark Tattrie
*/
#include "Weapon.h"
#include <iostream>
#include <atomic>

#include "../../log/log.h"
#include "../../audio/AudioManager.h"
#include "../../game/GameManager.h"
#include "../../sprites/Renderer.h"

using std::string;

/**
 * Date: Feb 8, 2017
 * Modified:
 *  Feb 9, 2017 - Jacob McPhail
 *  Mar 13, 2017 - Mark Tattrie
 * Author: Maitiu Morton
 * Function Interface: Weapon::Weapon(const string& type, TEXTURES sprite, const string& fireSound,
 *       const string& hitSound, const string& reloadSound, const string& emptySound, const int range,
 *       const int damage, const int AOE, const int penetration, const int clip, const int clipMax,
 *       const int ammo, const int reloadDelay, const int fireDelay, int32_t id)
 * Description:
 * Ctor for Weapon
 */
Weapon::Weapon(const string& type, TEXTURES sprite, const string& fireSound, const string& hitSound, const string& reloadSound,
        const string& emptySound, const int range, const int damage, const int AOE, const int penetration, const int accuracy,
        const int clip, const int clipMax, const int ammo, const int reloadDelay, const int fireDelay, const int texX,
        const int texY, int32_t id, const int p) : weaponSrc({texX, texY, WEAPON_WIDTH, WEAPON_HEIGHT}),rotate{0, 0},
        type(type), spriteType(sprite), fireSound(fireSound), hitSound(hitSound), reloadSound(reloadSound), emptySound(emptySound),
        range(range), damage(damage), AOE(AOE), penetration(penetration), accuracy(accuracy), clip(clip), clipMax(clipMax),
        ammo(ammo), reloadDelay(reloadDelay), fireDelay(fireDelay), reloadTick(0), fireTick(0),  wID(id), price(p){
}

Weapon::Weapon(const Weapon& w)
        : weaponSrc(w.weaponSrc), rotate(w.rotate), type(w.type), spriteType(w.spriteType), fireSound(w.fireSound),
        hitSound(w.hitSound), reloadSound(w.reloadSound), emptySound(w.emptySound), range(w.range), damage(w.damage), AOE(w.AOE),
        penetration(w.penetration), accuracy(w.accuracy), clip(w.clip), clipMax(w.clipMax), ammo(w.ammo),
        reloadDelay(w.reloadDelay), fireDelay(w.fireDelay), reloadTick(w.reloadTick), fireTick(w.fireTick), wID(w.getID()), price(w.getPrice()){
}


//Deric M       3/3/2017
bool Weapon::reduceClip(const int rounds){
    if(clip < rounds){
        reloadClip();
        return false;
    }
    clip -= rounds;
    return true;
}


//Mark T    3/8/2017
//Deric M       3/15/2017
bool Weapon::reloadClip(){
    int currentTime = SDL_GetTicks();
    if(currentTime < (reloadTick + reloadDelay)){
        return false;
    }
    reloadTick = currentTime;
    //must wait extra time to fire from reloading
    fireTick += reloadDelay;
    if(ammo <= 0){
        AudioManager::instance().playEffect(emptySound.c_str());
        return false;
    }

    int ammoNeeded = clipMax - clip;
    if(ammo < ammoNeeded){
        clip += ammo;
        ammo = 0;
        return true;
    }
    ammo -= ammoNeeded;
    clip += ammoNeeded;
    AudioManager::instance().playEffect(reloadSound.c_str());

    return true;
}


//Mark T    3/8/2017
//Deric M       3/15/2017
bool Weapon::chamberRound() {
    int currentTime = SDL_GetTicks();
    if(currentTime < (fireTick + fireDelay)){
        return false;
    }
    fireTick = currentTime;
    if(!reduceClip(1)){
        return false;
    }
    logv(3, "Current ammo: %d/%d\n", clip, ammo + clip);
    return true;
}

//Deric M       3/15/2017
bool Weapon::fire(Movable& movable){
    logv(3, "Weapon::fire()\n");
    if(!chamberRound()){
        return false;
    }
    if (networked) {
        GameManager::instance()->getPlayer().sendServAttackAction();
    }
    AudioManager::instance().playEffect(fireSound.c_str());
    return true;
}

void Weapon::updateGunRender(const Movable& mov, const SDL_Rect& camera) {
    static constexpr int WEAPON_DISP_WIDTH = 100;
    static constexpr int WEAPON_DISP_HEIGHT = 60;
    const auto& dest = mov.getRelativeDestRect(camera);

    weaponDest.x = dest.x + dest.w / 2;
    weaponDest.y = dest.y + dest.h / 2;
    weaponDest.w = WEAPON_DISP_WIDTH;
    weaponDest.h = WEAPON_DISP_HEIGHT;

    const double normal = mov.getAngle() - 90;
    //these angles are checking to make sure the angle is on the left half of the character
    if (-90 > normal && -270 < normal) {
        rotate.y = weaponDest.h / 2;
        Renderer::instance().render(weaponDest, TEXTURES::WEAPONS,
            weaponSrc, normal, &rotate, SDL_FLIP_VERTICAL);
    //likewise this is the right half
    } else {
        rotate.y = weaponDest.h / 2;
        Renderer::instance().render(weaponDest, TEXTURES::WEAPONS,
            weaponSrc, normal, &rotate);
    }
}
