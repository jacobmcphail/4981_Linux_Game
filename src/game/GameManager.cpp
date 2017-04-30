#include <omp.h>
#include <memory>
#include <utility>
#include <atomic>
#include <cassert>

#include "../collision/HitBox.h"
#include "../log/log.h"
#include "../game/GameManager.h"
#include "../sprites/Renderer.h"
#include "../buildings/WeaponStore.h"
#include "../server/servergamestate.h"
#include "../buildings/TechStore.h"
#include "../buildings/HealthStore.h"
Weapon w;
GameManager GameManager::sInstance;
//Returns the already existing GameManager or if there isn't one, makes
//a new one and returns it.
GameManager *GameManager::instance() {
    return &GameManager::sInstance;
}

int32_t GameManager::generateID() {
    static std::atomic<int32_t> counter{-1};
    return ++counter;
}

/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: GameManager()
 * Description:
 *     ctor for the game manager.
 */
GameManager::GameManager() : collisionHandler() {
    logv("Create GM\n");
}

/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: ~GameManager()
 * Description:
 *     dctor for the game manager.
 */
GameManager::~GameManager() {
    logv("Destroy GM\n");
}

/**
 * Date: Feb. 4, 2017
 *
 * Author: Jacob McPhail
 *
 * Modified: Mar. 15, 2017 - Mark Tattrie
 * Modified: Apr. 02, 2017 - Terry Kang
 *  Set alpha to the sprite of Brricade if it is not placeable
 * Modified: Apr. 07, 2017 - Isaac Morneau
 *      cleaned up the inersection calls, removed object rendering entirely
 *
 * Function Interface: void GameManager::renderObjects(const SDL_Rect& cam)
 *
 * Description:
 *     Render all objects in level
 */
void GameManager::renderObjects(const SDL_Rect& cam) {
    for (const auto& o : weaponDropManager) {
        if (SDL_HasIntersection(&cam, &o.second.getDestRect())) {
            Renderer::instance().render(o.second.getRelativeDestRect(cam),
                getWeapon(o.second.getWeaponId())->getTexture());
        }
    }

    for (const auto& o: barricadeDropManager) {
        if (SDL_HasIntersection(&cam, &o.second.getDestRect())) {
            Renderer::instance().render(o.second.getRelativeDestRect(cam),
                TEXTURES::MAP_OBJECTS, o.second.getSrcRect());
        }
    }

    for (const auto& o : consumeDropManager) {
        if (SDL_HasIntersection(&cam, &o.second.getDestRect())) {
            Renderer::instance().render(o.second.getRelativeDestRect(cam),
                TEXTURES::HEALTHPACK);
        }
    }

    for (const auto& o : marineManager) {
        if (SDL_HasIntersection(&cam, &o.second.getDestRect())) {
            const auto& dest = o.second.getRelativeDestRect(cam);
            const auto angle = o.second.getAngle() - 90;

            if (-180 < angle && 0 > angle) {
                Weapon* weapon = o.second.inventory.getCurrent();
                if (weapon) {
                    weapon->updateGunRender(o.second, cam);
                }
                Renderer::instance().render(dest,
                    o.second.getId() % 2 ? TEXTURES::MARINE : TEXTURES::COWBOY,
                    o.second.getSrcRect());
            } else {
                Renderer::instance().render(dest,
                    o.second.getId() % 2 ? TEXTURES::MARINE : TEXTURES::COWBOY,
                    o.second.getSrcRect());
                Weapon* weapon = o.second.inventory.getCurrent();
                if (weapon) {
                    weapon->updateGunRender(o.second, cam);
                }
            }
        }
    }

    if (SDL_HasIntersection(&cam, &base.getDestRect())) {
        Renderer::instance().render(base.getRelativeDestRect(cam), TEXTURES::BASE,
            base.getSrcRect());
    }

    for (const auto& o : zombieManager) {
        if (SDL_HasIntersection(&cam, &o.second.getDestRect())) {
            Renderer::instance().render(o.second.getRelativeDestRect(cam),
                    o.second.getId() % 2 ? TEXTURES::BABY_ZOMBIE : TEXTURES::DIGGER_ZOMBIE,
                    o.second.getSrcRect());
        }
    }

    for (const auto& o : turretManager) {
        if (SDL_HasIntersection(&cam, &o.second.getDestRect())) {

            if (!o.second.isPlaceable()) {
                Renderer::instance().setAlpha(TEXTURES::TURRET, 150);
                Renderer::instance().render(o.second.getRelativeDestRect(cam), TEXTURES::TURRET);
                Renderer::instance().setAlpha(TEXTURES::TURRET, 255);
            } else {
                Renderer::instance().render(o.second.getRelativeDestRect(cam), TEXTURES::TURRET);
            }

        }
    }

    for (const auto& o : barricadeManager) {
        if (SDL_HasIntersection(&cam, &o.second.getDestRect())) {
            if(!o.second.isPlaceable()) {
                Renderer::instance().setAlpha(TEXTURES::CONCRETE, 150);
                Renderer::instance().render(o.second.getRelativeDestRect(cam), TEXTURES::MAP_OBJECTS, o.second.getSrcRect());
                Renderer::instance().setAlpha(TEXTURES::MAP_OBJECTS, 255);
            } else {
                Renderer::instance().render(o.second.getRelativeDestRect(cam), TEXTURES::MAP_OBJECTS, o.second.getSrcRect());
            }
        }
    }

    for (const auto& o : wallManager) {
        if (SDL_HasIntersection(&cam, &o.second.getDestRect())) {
            static constexpr SDL_Rect WALL_SRC_RECT = {WALL_SRC_X, WALL_SRC_Y, WALL_SRC_W, WALL_SRC_H};
            Renderer::instance().render(o.second.getRelativeDestRect(cam), TEXTURES::MAP_OBJECTS,
                WALL_SRC_RECT, WALL_WIDTH, WALL_HEIGHT);
        }
    }

    for (const auto& o : storeManager) {
        if (SDL_HasIntersection(&cam, &o.second->getDestRect())) {
            Renderer::instance().render(o.second->getRelativeDestRect(cam), TEXTURES::MAP_OBJECTS,
                o.second->getSrcRect());
        }
        if(o.second->isOpen()){
            o.second->getStoreMenu().renderBackground();
        }
    }
}

/**
 * Date: April. 8, 2017
 *
 * Author: Maitiu
 * Function Interface: void GameManager::updateStores()
 *
 * Description:
 *     Updates all the stores, Checks if player is still touching store if store is open
 */
void GameManager::updateStores(){
    for(auto& s : storeManager){
        if(s.second->isOpen()){
            if(!collisionHandler.detectStoreCollision(static_cast<Entity*>(player.getMarine()), static_cast<Entity*>(s.second.get()))){
                s.second->closeStore();
                player.getMarine()->leaveStore();
            }
        }
    }
}
/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: (const float delta)
 *      delta : Delta time to control frame rate.
 *
 * Description:
 *     Update marine movements. health, and actions
 */
void GameManager::updateMarines(const float delta) {
#pragma omp parallel
#pragma omp single
    {
        for (auto it = marineManager.begin(); it != marineManager.end(); ++it) {
#pragma omp task firstprivate(it)
            {
                if (!networked) {
                    it->second.move((it->second.getDX() * delta), (it->second.getDY() * delta), collisionHandler);
                }
#ifndef SERVER
                it->second.updateImageDirection();
                it->second.updateImageWalk();
#endif
            }
        }
#pragma omp taskwait
    }
}

// Update zombie movements.
void GameManager::updateZombies(const float delta) {
#pragma omp parallel
#pragma omp single
    {
        for (auto it = zombieManager.begin(); it != zombieManager.end(); ++it) {
#pragma omp task firstprivate(it)
            {
                it->second.update();
                it->second.move((it->second.getDX() * delta), (it->second.getDY() * delta), collisionHandler);
#ifndef SERVER
                it->second.updateImageDirection();
                it->second.updateImageWalk();
#endif
            }
        }
#pragma omp taskwait
    }
}

/**
* Date: April 6, 2017
* Designer: Trista Huang
* Programmer: Trista Huang
* Function Interface: void GameManager::updateBase()
* Description:
*       This function calls function to check for base health everytime an update happens,
*       and changes base image accordingly.
*       It is called from GameStateMatch every update.
*/
void GameManager::updateBase() {
    base.updateBaseImage();
}

bool GameManager::hasMarine(const int32_t id) const {
    return marineManager.count(id);
}


/**
 * Date: Mar. 01, 2017
 * Modified: Mar. 30, 2017 - Mark Chen
 *           Apr. 05, 2017 - Mark Chen
 * Designer: Jamie Lee
 *
 * Programmer: Jamie Lee, Mark Chen
 *
 * Function Interface: void updateTurrets()
 *
 * Description:
 * Updates the turrets actions.
 *
 * Revisions:
 * Mar. 30, 2017, Mark Chen : turrets now fire when they detect an enemy
 * Apr. 05, 2017, Mark Chen : turrets get deleted when their ammo reaches 0.
 * Apr. 10, 2017, Mark Chen : turrets now do not track targets while it's being held.
 */

void GameManager::updateTurrets() {
    std::vector<int32_t> deleteVector = markForDeletionTurret();

    for (auto it = deleteVector.begin() ; it != deleteVector.end(); ++it) {
        removeWeapon(getTurret(*it).getInventory().getCurrent()->getID());
        deleteTurret(*it);
    }

#pragma omp parallel
#pragma omp single
    {
        for (auto it = turretManager.begin(); it != turretManager.end(); ++it) {
#pragma omp task firstprivate(it)
            {
                if (it->second.isActivated() && it->second.targetScanTurret()) {
                    it->second.shootTurret();
                }
            }
        }
#pragma omp taskwait
    }
}

/**
 * Date: Apr. 05, 2017
 *
 * Designer: Mark Chen
 *
 * Programmer: Mark Chen
 *
 * Function Interface: vector<int32_t> GameManager::markForDeletionTurret()
 *
 * Description:
 * Searches the turretManager for any turrets with 0 ammo.
 */

std::vector<int32_t> GameManager::markForDeletionTurret() {
    std::vector<int32_t> deleteVector;
    for (auto& t: turretManager) {
        if (t.second.getInventory().getCurrent()->getClip() == 0) {
            deleteVector.push_back(t.second.getId());
        }
    }
    return deleteVector;
}

/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: createMarine()
 * Description:
 *     Create marine add it to manager, returns marine id
 */
int32_t GameManager::createMarine() {
    const int32_t id = generateID();
    SDL_Rect temp = {INITVAL, INITVAL, MARINE_WIDTH, MARINE_HEIGHT};

    SDL_Rect marineRect = temp;
    SDL_Rect moveRect = temp;
    SDL_Rect projRect = temp;
    SDL_Rect damRect = temp;

    marineManager.emplace(id, Marine(id, marineRect, moveRect, projRect, damRect));
    return id;
}

/**
 * Date: Mar. 1, 2017
 * Modified: Mar. 15 2017 - Mark Tattrie
 * Author: Jacob McPhail
 * Function Interface: bool GameManager::createMarine(const float x, const float y) {
 * Description:
 *     Create a marine at position x,y and add it to the marine manager
 */
bool GameManager::createMarine(const float x, const float y) {
    const int32_t id = generateID();
    SDL_Rect temp = {INITVAL, INITVAL, MARINE_WIDTH, MARINE_HEIGHT};

    SDL_Rect marineRect = temp;
    SDL_Rect moveRect = temp;
    SDL_Rect projRect = temp;
    SDL_Rect damRect = temp;

    const auto& elem = marineManager.emplace(id, Marine(id, marineRect, moveRect, projRect, damRect));
    elem->second.setPosition(x,y);
    return true;
}

void GameManager::createMarine(const int32_t id) {
    SDL_Rect temp = {INITVAL, INITVAL, MARINE_WIDTH, MARINE_HEIGHT};

    SDL_Rect marineRect = temp;
    SDL_Rect moveRect = temp;
    SDL_Rect projRect = temp;
    SDL_Rect damRect = temp;

    marineManager.emplace(id, Marine(id, marineRect, moveRect, projRect, damRect));
}

/**
 * Date: Mar. 1, 2017
 * Modified: Mar. 15 2017 - Mark Tattrie
 * Author:
 * Function Interface: void GameManager::deleteMarine(const int32_t id) {
 * Description:
 * remove the marine by its id from the marineManager
 */
void GameManager::deleteMarine(const int32_t id) {
    marineManager.erase(id);
#ifdef SERVER
    saveDeletion({UDPHeaders::MARINE, id});
#endif
}

/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: addMarine(const int32_t id, const Marine& newMarine)
 *      id : Marine id
 *      newMarine : Marine to add the manager
 *
 * Description:
 *     Adds marine to level.
 */
bool GameManager::addMarine(const int32_t id, const Marine& newMarine) {
    if (marineManager.count(id)) {
        return false;
    }
    marineManager.emplace(id, newMarine);
    return true;
}

/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: getMarine(const int32_t id)
 *      id : Marine id
 *
 * Description:
 *     Get a marine by its id
 */
/*
Marine& GameManager::getMarine(const int32_t id) {
    const auto& mar = marineManager[id];
    assert(mar.second);
    return mar.first;
}*/

/**
 * Date: Feb. 9, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: createTurret()
 * Description:
 *     Create Turret add it to manager, returns tower id.
 */
int32_t GameManager::createTurret() {
    const int32_t id = generateID();
    SDL_Rect temp = {INITVAL, INITVAL, DEFAULT_SIZE, DEFAULT_SIZE};

    SDL_Rect turretRect = temp;
    SDL_Rect moveRect = temp;
    SDL_Rect projRect = temp;
    SDL_Rect damRect = temp;
    SDL_Rect pickRect = temp;

    turretManager.emplace(id, Turret(id, turretRect, moveRect, projRect, damRect, pickRect));
    return id;
}

/**
 * Date: Feb. 9, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: deleteTurret(const int32_t id)
 *      id : Turret id
 *
 * Description:
 *     Deletes tower from level.
 */
void GameManager::deleteTurret(const int32_t id) {
    turretManager.erase(id);
#ifdef SERVER
    saveDeletion({UDPHeaders::TURRET, id});
#endif
}

/**
 * Date: Feb. 9, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: addTurret (const int32_t id, const Turret& newTurret)
 *      id : Turret id
 *      newTurret : Turret to add
 *
 * Description:
 *     Adds tower to level.
 */
bool GameManager::addTurret (const int32_t id, const Turret& newTurret) {
    if (turretManager.count(id)) {
        return false;
    }
    turretManager.emplace(id, newTurret);
    return true;
}

/**
 * Date: Mar. 1, 2017
 * Modified: Mar. 15 2017 - Mark Tattrie
 * Author:
 * Function Interface: int32_t GameManager::createTurret(const float x, const float y) {
 * Description:
 * Create turret add it to turret, returns if success
 */
int32_t GameManager::createTurret(const float x, const float y) {
    const int32_t id = generateID();
    SDL_Rect temp = {INITVAL, INITVAL, DEFAULT_SIZE, TURRET_SIZE_H};

    SDL_Rect turretRect = temp;
    SDL_Rect moveRect = temp;
    SDL_Rect projRect = temp;
    SDL_Rect damRect = temp;
    SDL_Rect pickRect = {INITVAL, INITVAL, TURRET_PUSIZE_W, TURRET_PUSIZE_H};

    const auto& elem = turretManager.emplace(id, Turret(id, turretRect, moveRect, projRect, damRect,
        pickRect));
    elem->second.setPosition(x,y);
    return id;
}

/**
 * Date: Feb. 9, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: getTurret(const int32_t id)
 *      id : Turret id
 *
 * Description:
 *      Get a tower by its id.
 */
Turret& GameManager::getTurret(const int32_t id) {
    const auto& turr = turretManager[id];
    assert(turr.second);
    return turr.first;
}

/**
 * Date: Feb. 8, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: addZombie(const Zombie& newZombie)
 *      newZombie : Zombie to add
 *
 * Description:
 *     Add a zombie to the manager.
 */
int32_t GameManager::addZombie(const Zombie& newZombie) {
    const int32_t id = generateID();
    zombieManager.emplace(id, newZombie);
    return id;
}

void GameManager::createZombie(const int32_t id) {
    SDL_Rect temp = {INITVAL, INITVAL, DEFAULT_SIZE, DEFAULT_SIZE};

    SDL_Rect zombieRect = temp;
    SDL_Rect moveRect = temp;
    SDL_Rect projRect = temp;
    SDL_Rect damRect = temp;

    zombieManager.emplace(id, Zombie(id, zombieRect, moveRect, projRect, damRect));
}

/**
* Date: Mar. 1, 2017
* Modified: Mar. 15 2017 - Mark Tattrie
* Author: Jacob McPhail
* Function Interface: bool GameManager::createZombie(const float x, const float y)
* Description:
*   Create zombie add it to manager, returns success
*/
int32_t GameManager::createZombie(const float x, const float y) {
    const int32_t id = generateID();
    SDL_Rect temp = {INITVAL, INITVAL, ZOMBIE_WIDTH, ZOMBIE_HEIGHT};

    SDL_Rect zombieRect = temp;
    SDL_Rect moveRect = temp;
    SDL_Rect projRect = temp;
    SDL_Rect damRect = temp;

    const auto& elem = zombieManager.emplace(id, Zombie(id, zombieRect, moveRect, projRect, damRect));
    elem->second.setPosition(x,y);
    return id;
}

/**
 * Date: Feb. 8, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: deleteZombie(const int32_t id)
 *      id : Zombie id
 *
 * Description:
 *     Deletes zombie from level.
 */
void GameManager::deleteZombie(const int32_t id) {
    zombieManager.erase(id);
#ifdef SERVER
    saveDeletion({UDPHeaders::ZOMBIE, id});
#endif
}

/*
    AUTHOR: Deric Mccadden 21/03/2017
    DESC: Checks if id can be found in zombieManager
 */
bool GameManager::zombieExists(const int32_t id) {
    return zombieManager.count(id);
}

/*
    AUTHOR: Deric Mccadden 21/03/2017
    DESC: returns zombie that matches id from zombieManager
 */
Zombie& GameManager::getZombie(const int32_t id) {
    const auto& z = zombieManager[id];
    assert(z.second);
    return z.first;
}
/**
 * Date: March. 30, 2017
 * Modified: ----
 * Author: MAitiu Morton
 * Function Interface: void GameManager::addWeapon(std::shared_ptr<Weapon> weapon)
 *      weapon: weapon to be added
 *
 * Description:
 *     Adds Weapon to weaponManager
 */
void GameManager::addWeapon(std::shared_ptr<Weapon> weapon) {
    weaponManager.emplace(weapon->getID(), weapon);
}

/**
 * Date: March. 30, 2017
 * Modified: ----
 * Author: MAitiu Morton
 * Function Interface: void GameManager::removeWeapon(const int32_t id)
 *      id: id of Weapon to be removed
 *
 * Description:
 *     removes Weapon from weaponManager
 */
void GameManager::removeWeapon(const int32_t id) {
    weaponManager.erase(id);
#ifdef SERVER
    saveDeletion({UDPHeaders::WEAPON, id});
#endif
}

/**
 * Date: March. 12, 2017
 * Modified: ----
 * Author: MAitiu Morton
 * Function Interface: int32_t GameManager::addWeaponDrop(WeaponDrop& newWeaponDrop)
 *      newWeaponDrop: WeaponDrop ti be added
 *
 * Description:
 *     addes WeaponDrop ro WaeponDrop manager
 */
int32_t GameManager::addWeaponDrop(WeaponDrop& newWeaponDrop) {
    const int32_t id = newWeaponDrop.getId();
    weaponDropManager.emplace(id, newWeaponDrop);
    return id;
}

/**
* Date: Mar. 3, 2017
* Modified: Mar. 15 2017 - Mark Tattrie
* Author: Maitiu Morton 2017-03-12
* Function Interface: bool GameManager::createWeaponDrop(const float x, const float y, const int32_t wID)
* Description:
* Create weapon drop add it to manager, returns success
*/
int32_t GameManager::createWeaponDrop(const float x, const float y, const int32_t wID) {
    const int32_t id = generateID();

    SDL_Rect weaponDropRect = {static_cast<int>(x),static_cast<int>(y), DEFAULT_SIZE, DEFAULT_SIZE};
    SDL_Rect pickRect = {static_cast<int>(x),static_cast<int>(y), DEFAULT_SIZE, DEFAULT_SIZE};

    weaponDropManager.emplace(id, WeaponDrop(id, weaponDropRect, pickRect, wID))->second.setPosition(x,y);
    return id;
}

/**
* Date: Mar. 21, 2017
* Author: Maitiu Morton
* Function Interface: bGameManager::weaponDropExists(const int32_t id)
*   id: id for Weapondrop
* Description:
* Checks if WeaponDrop Exits
*/
bool GameManager::weaponDropExists(const int32_t id) {
    return weaponDropManager.count(id);
}

/**
* Date: Mar. 21, 2017
* Author: Maitiu Morton
* Function Interface: WeaponDrop& GameManager::getWeaponDrop(const int32_t id)
*   id: id for Weapondrop
* Description:
*   Gets Weapondrop from Weapondrop Manager
*/
WeaponDrop& GameManager::getWeaponDrop(const int32_t id) {
    logv("id: %d\n", id);
    const auto& wd = weaponDropManager[id];
    assert(wd.second);
    return wd.first;
}

/**
* Date: Mar. 12, 2017
* Author: Maitiu Morton
* Function Interface: std::shared_ptr<Weapon> GameManager::getWeapon(const int32_t id)
*   id: id for Weapon
* Description:
*   Gets Weapon from Weapon Manager
*/
std::shared_ptr<Weapon> GameManager::getWeapon(const int32_t id) {
    const auto& w = weaponManager[id];
    assert(w.second);
    return w.first;
}

/**
* Date: Mar. 12, 2017
* Author: Maitiu Morton
* Function Interface: void GameManager::deleteWeaponDrop(const int32_t id)
*   id: id for WeaponDrop
* Description:
*  Removes WeaponDrop From WeaponDropManager
*/
void GameManager::deleteWeaponDrop(const int32_t id) {
    weaponDropManager.erase(id);
#ifdef SERVER
    saveDeletion({UDPHeaders::WEAPONDROP, id});
#endif
}

/**
* Date: April. 9, 2017
* Author: Maitiu Morton
* Function Interface: void GameManager::addConsumable(std::shared_ptr<Consumable> consumable)
*   Consumable: consumable to be added
* Description:
*  adds Consumable to ConsumeManager
*/
void GameManager::addConsumable(std::shared_ptr<Consumable> consumable) {
    consumableManager.emplace(consumable->getId(), consumable);
}

/**
* Date: March 30, 2017
* Author: Maitiu Morton
* Function Interface: bool GameManager::consumableExists(const int32_t id)
*      id: id of consumable
* Description:
*  Checks if consumable exists
*/
bool GameManager::consumableExists(const int32_t id) {
   return consumableManager.count(id);
}

/**
* Date: April. 9, 2017
* Author: Maitiu Morton
* Function Interface: void GameManager::removeConsumable(const int32_t id)
*   id: of consumable to be removed
* Description:
*  Removes Consumable from ConsumeManager
*/
void GameManager::removeConsumable(const int32_t id) {
    consumableManager.erase(id);
/*#ifdef SERVER
    saveDeletion({UDPHeaders::CONSUMABLE, id});
#endif*/
}

/**
* Date: April. 9, 2017
* Author: Maitiu Morton
* Function Interface: void GameManager::removeConsumable(const int32_t id)
*   id: of consumable to be removed
* Description:
*  Removes Consumable from ConsumeManager
*/
std::shared_ptr<Consumable> GameManager::getConsumable(const int32_t id) {
    const auto& c = consumableManager[id];
    assert(c.second);
    return c.first;
}

/**
* Date: April. 9, 2017
* Author: Maitiu Morton
* Function Interface: int32_t GameManager::createConsumeDrop(const float x, const float y, const int32_t cID)
*   x: x coordinate for drop to be rendered
*   y: y coordinte for drop to be rendered
*   cID: id for consumable that will be held in the drop
* Description:
*  Creates and added a cosumeDrop to consumdrop manager
*/
int32_t GameManager::createConsumeDrop(const float x, const float y, const int32_t cID) {
    const int32_t id = generateID();

    SDL_Rect consumeDropRect = {static_cast<int>(x),static_cast<int>(y), DEFAULT_SIZE, DEFAULT_SIZE};
    SDL_Rect pickRect = {static_cast<int>(x),static_cast<int>(y), DEFAULT_SIZE, DEFAULT_SIZE};
    consumeDropManager.emplace(id, ConsumeDrop(id, consumeDropRect, pickRect, cID))->second.setPosition(x,y);
    return id;
}

/**
* Date: April. 9, 2017
* Author: Maitiu Morton
* Function Interface: bool GameManager::consumeDropExists(const int32_t id)
*  id:id of consumedrop
* Description:
*  checks if id is in the consumedrop manager
*/
bool GameManager::consumeDropExists(const int32_t id) {
    return consumeDropManager.count(id);
}

/**
* Date: April. 9, 2017
* Author: Maitiu Morton
* Function Interface: ConsumeDrop& GameManager::getConsumeDrop(const int32_t id)
*  id:id of consumedrop
* Description:
*  gets consumedrop from consumdrop manager
*/
ConsumeDrop& GameManager::getConsumeDrop(const int32_t id) {
    logv("id: %d\n", id);
    const auto& cd = consumeDropManager[id];
    assert(cd.second);
    return cd.first;
}

/**
* Date: April. 9, 2017
* Author: Maitiu Morton
* Function Interface: void GameManager::deleteConsumeDrop(const int32_t id)
*  id:id of consumedrop
* Description:
*  gdeletes consumedrop from consumdrop manager
*/
void GameManager::deleteConsumeDrop(const int32_t id) {
    consumeDropManager.erase(id);
}

 /**
 * Date: March 30, 2017
 * Author: Maitiu Morton
 * Revised By Michael Goll [April 4, 2017] - Added sprite for store.
 * Function Interface: int32_t GameManager::createWeaponStore(const float x, const float y, SDL_Rect screenRect)
 *      x: x coordinate to render
 *      y: y coordinate to render
 *      screenRect: used for store menu
 * Description:
 *  Creates a Weapon store object and then calls addStore to add it to the manager.
 */
int32_t GameManager::createWeaponStore(const float x, const float y, SDL_Rect screenRect) {
    const int32_t id = generateID();
    GameHashMap<TEXTURES, int> gh;
    SDL_Rect weaponStoreRect = {static_cast<int>(x),static_cast<int>(y), STORE_SIZE_W, STORE_SIZE_H};
    SDL_Rect pickRect = {static_cast<int>(x) - STORE_PICKUP_SIZE / 2, static_cast<int>(y) - STORE_PICKUP_SIZE / 2,
            STORE_SIZE_W + STORE_PICKUP_SIZE, STORE_SIZE_H + STORE_PICKUP_SIZE};

    gh.emplace(TEXTURES::RIFLE, 0);

    std::shared_ptr<WeaponStore> ws = std::make_shared<WeaponStore>(id, weaponStoreRect, pickRect, screenRect, gh);
    addStore(id, std::dynamic_pointer_cast<Store>(ws));
    ws->setSrcRect(WEAPON_STORE_SRC_X, WEAPON_STORE_SRC_Y, WEAPON_STORE_SRC_W, WEAPON_STORE_SRC_H);

    return id;
}

/**
* Date: March 30, 2017
* Author: Maitiu Morton
* Function Interface: int32_t GameManager::createTechStore(const float x, const float y, SDL_Rect screenRect)
*      x: x coordinate to render
*      y: y coordinate to render
*      screenRect: used for store menu
* Description:
*  Creates a Tech store object and then calls addStore to add it to the manager.
*/
int32_t GameManager::createTechStore(const float x, const float y, SDL_Rect screenRect) {
    const int32_t id = generateID();
    GameHashMap<TEXTURES, int> gh;
    SDL_Rect techStoreRect = {static_cast<int>(x),static_cast<int>(y), STORE_SIZE_W, STORE_SIZE_H};
    SDL_Rect pickRect = {static_cast<int>(x) - STORE_PICKUP_SIZE / 2, static_cast<int>(y) - STORE_PICKUP_SIZE / 2,
            STORE_SIZE_W + STORE_PICKUP_SIZE, STORE_SIZE_H + STORE_PICKUP_SIZE};

    gh.emplace(TEXTURES::RIFLE, 0);

    std::shared_ptr<TechStore> ws = std::make_shared<TechStore>(id, techStoreRect, pickRect, screenRect, gh);
    addStore(id, std::dynamic_pointer_cast<Store>(ws));
    ws->setSrcRect(TECH_STORE_SRC_X, TECH_STORE_SRC_Y, TECH_STORE_SRC_W, TECH_STORE_SRC_H);

    return id;
}


/**
* Date: March 30, 2017
* Author: Maitiu Morton
* Function Interface: int32_t GameManager::createTechStore(const float x, const float y, SDL_Rect screenRect)
*      x: x coordinate to render
*      y: y coordinate to render
*      screenRect: used for store menu
* Description:
*  Creates a Health store object and then calls addStore to add it to the manager.
*/
int32_t GameManager::createHealthStore(const float x, const float y, SDL_Rect screenRect) {

    const int32_t id = generateID();
    GameHashMap<TEXTURES, int> gh;
    SDL_Rect healthStoreRect = {static_cast<int>(x),static_cast<int>(y), STORE_SIZE_W, STORE_SIZE_H};
    SDL_Rect pickRect = {static_cast<int>(x) - STORE_PICKUP_SIZE / 2, static_cast<int>(y) - STORE_PICKUP_SIZE / 2,
            STORE_SIZE_W + STORE_PICKUP_SIZE, STORE_SIZE_H + STORE_PICKUP_SIZE};

    gh.emplace(TEXTURES::RIFLE, 0);

    std::shared_ptr<HealthStore> ws = std::make_shared<HealthStore>(id, healthStoreRect, pickRect, screenRect, gh);
    addStore(id, std::dynamic_pointer_cast<Store>(ws));
    ws->setSrcRect(HEALTH_STORE_SRC_X, HEALTH_STORE_SRC_Y, HEALTH_STORE_SRC_W, HEALTH_STORE_SRC_H);

    return id;
}

/**
* Date: MArch 30, 2017
* Author: Maitiu Morton
* Function Interface:  void GameManager::addStore(const int32_t id ,std::shared_ptr<Store> store)
*      id: id of store to be added
*      store: store that is to be added
* Description:
*  Adds store to store manager.
*/
 void GameManager::addStore(const int32_t id ,std::shared_ptr<Store> store) {
     storeManager.emplace(id, store);
 }

 /**
 * Date: MArch 30, 2017
 * Author: Maitiu Morton
 * Function Interface:   bool GameManager::storeExists(const int32_t id)
 *      id: id of store
 * Description:
 *  Checks if store exists in store manager
 */
 bool GameManager::storeExists(const int32_t id) {
     return storeManager.count(id);
 }

 /**
 * Date: MArch 30, 2017
 * Author: Maitiu Morton
 * Function Interface:  std::shared_ptr<Store> GameManager::getStore(const int32_t id)
 *      id: id of store
 * Description:
 *  Cgets a store from the store manager
 */
 std::shared_ptr<Store> GameManager::getStore(const int32_t id) {
     const auto& s = storeManager[id];
     assert(s.second);
     return s.first;
 }

/*
 * created by Maitiu March 31
 * creates a square area of DropPoints
 */
void GameManager::createDropZone(const float x, const float y, const int num) {
    dropZoneCoord.first = x;
    dropZoneCoord.second = y;
    for (int i = 0; i < num; i++) {
        for (int j = 0; j < num; j++) {
            createDropPoint(x + (DROP_POINT_SPACE * i), y + (DROP_POINT_SPACE * j));
        }
    }
}

/**
* Date: April8, 2017
* Author: Maitiu Morton
* Function Interface:  int32_t GameManager::createBarricadeDrop(const float x, const float y)
*      x: x corrdinates where entity will be rendered
*      Y: y coordinate where entity will be rendered
* Description:
*  creates and puts barricade drop in barricadeDropManager
*/
int32_t GameManager::createBarricadeDrop(const float x, const float y){
    const int32_t id = generateID();

    SDL_Rect barricadeDropRect = {static_cast<int>(x),static_cast<int>(y), DEFAULT_SIZE, DEFAULT_SIZE};
    SDL_Rect pickRect = {static_cast<int>(x),static_cast<int>(y), DEFAULT_SIZE, DEFAULT_SIZE};

    barricadeDropManager.emplace(id, BarricadeDrop(id, barricadeDropRect, pickRect))->second.setPosition(x,y);
    getBarricadeDrop(id).setSrcRect(WALL_SRC_X, WALL_SRC_Y, WALL_SRC_W, WALL_SRC_H);
    return id;
}

/**
* Date: April8, 2017
* Author: Maitiu Morton
* Function Interface:  int32_t GameManager::createBarricadeDrop(const float x, const float y)
*      id: id of barricade Drop
* Description:
*  Gets a barricade Drop from the barricadDrop manager
*/
BarricadeDrop& GameManager::getBarricadeDrop(const int32_t id) {
    const auto& bd = barricadeDropManager[id];
    assert(bd.second);
    return bd.first;
}

/**
* Date: April8, 2017
* Author: Maitiu Morton
* Function Interface: void GameManager::deleteBarricadeDrop(int32_t id)
*      id: id of barricade Drop
* Description:
*  Deletes barricade drop from barricade manager
*/
void GameManager::deleteBarricadeDrop(int32_t id){
        barricadeDropManager.erase(id);
}

/**
* Date: March 30, 2017
* Author: Maitiu Morton
* Function Interface: int32_t GameManager::createDropPoint(const float x, const float y)
*      x: x coordinat of drop point
*      y: y coordinate of drop point
* Description:
*  creates a drop point and adds it to the drop point manager
*/
 int32_t GameManager::createDropPoint(const float x, const float y) {
     const int32_t id = generateID();
     dropPointManager.emplace(id, DropPoint(id, x, y));
     openDropPoints.push_back(id);
     return id;
 }

 /**
 * Date: March 30, 2017
 * Author: Maitiu Morton
 * Function Interface: bool GameManager::dropPointExists(const int32_t id)
 *      id: id of drop point
 * Description:
 *  Checks if drop point exists
 */
bool GameManager::dropPointExists(const int32_t id) {
    return dropPointManager.count(id);
}

/**
* Date: March 30, 2017
* Author: Maitiu Morton
* Function Interface: bool GameManager::checkFreeDropPoints()
*
* Description:
*  Checks for free drop points
*/
bool GameManager::checkFreeDropPoints() {
    return !openDropPoints.empty();
}

/**
* Date: March 30, 2017
* Author: Maitiu Morton
* Function Interface:int32_t GameManager::getFreeDropPointId()
*
* Description:
*  Returns id of a free Drop Point
*/
int32_t GameManager::getFreeDropPointId() {
     const int32_t id = openDropPoints.back();
     openDropPoints.pop_back();
     return id;
}

/**
* Date: March 30, 2017
* Author: Maitiu Morton
* Function Interface: void GameManager::freeDropPoint(const int32_t id)
*       id: id of drop point
* Description:
*  puts a drop point in the open drop point map
*/
void GameManager::freeDropPoint(const int32_t id) {
    openDropPoints.push_back(id);
}

/**
* Date: March 30, 2017
* Author: Maitiu Morton
* Function Interface:DropPoint& GameManager::getDropPoint(const int32_t id)
*       id: id of drop point
* Description:
*  gets a drop point from the drop point manager
*/
DropPoint& GameManager::getDropPoint(const int32_t id) {
    const auto& s = dropPointManager[id];
    assert(s.second);
    return s.first;
}

// Returns Collision Handler
CollisionHandler& GameManager::getCollisionHandler() {
    return collisionHandler;
}

/**
 * Date: Feb. 4, 2017
 * Modified: Mar. 15, 2017 - Mark Tattrie
 * Modified: Apr. 5, 2017 - John Agapeyev
 *      added openMP
 * Modified: Apr. 7, 2017 - Isaac Morneau
 *      removed object manager, added base to walls
 * Author: Jacob McPhail
 * Function Interface: void GameManager::updateCollider()
 * Description:
 *     Update colliders to current state
 */
void GameManager::updateCollider() {
    collisionHandler.clear();

    //adding the base to the wall manager
    //this way we dont need the object manager at all
    collisionHandler.insertWall(&base);

#pragma omp parallel sections shared(collisionHandler)
    {
#pragma omp section
        for (auto& m : marineManager) {
            collisionHandler.insertMarine(&m.second);
        }

#pragma omp section
        for (auto& z : zombieManager) {
            collisionHandler.insertZombie(&z.second);
        }

#pragma omp section
        for (auto& w : wallManager) {
            collisionHandler.insertWall(&w.second);
        }

#pragma omp section
        for (auto& m : turretManager) {
            if (m.second.isPlaced()) {
                collisionHandler.insertTurret(&m.second);
            }
        }

#pragma omp section
        for (auto& b : barricadeManager) {
            if (b.second.isPlaced()) {
                collisionHandler.insertBarricade(&b.second);
            }
        }

#pragma omp section
        for (auto& m : weaponDropManager) {
            collisionHandler.insertPickUp(&m.second);
        }
#pragma omp section
        for (auto& bd : barricadeDropManager) {
            collisionHandler.insertPickUp(&bd.second);
        }
#pragma omp section
        for (auto& cd : consumeDropManager) {
            collisionHandler.insertPickUp(&cd.second);
        }

#pragma omp section
        for (auto& s : storeManager) {
            collisionHandler.insertStore(s.second.get());
            collisionHandler.insertPickUp(s.second.get());
        }
    }
}

/**
Date: 30. 17, 2017
Programmer: Brody McCrone
Interface: void GameManager::updateMarine(const PlayerData &playerData)
    playerData: Player data struct received from the server containing
        updated player info.
Description:
Checks if there is a marine in the marineManager with the id in the
playData struct, if not it creates a marine with that id. Whether it
created it or not it updates it's positition angle and health.
*/
void GameManager::updateMarine(const PlayerData &playerData) {
    if (marineManager.count(playerData.playerid) == 0) {
        createMarine(playerData.playerid);
    }
    Marine& marine = marineManager[playerData.playerid].first;
    marine.setPosition(playerData.xpos, playerData.ypos);
    marine.setDX(playerData.xdel);
    marine.setDY(playerData.ydel);
    marine.setAngle(playerData.direction);
    marine.setHealth(playerData.health);
}

/**
Date: 30. 17, 2017
Programmer: Brody McCrone
Interface: void GameManager::updateZombie(const ZombieData &zombieData)
    zobmieData: Zombie data struct received from the server containing
        updated zombie info.
Description:
Checks if there is a zombie in the zombieManager with the id in the
playData struct, if not it creates that zombie with that id. Whether
it created it or not it updates it's positition angle and health.
*/
void GameManager::updateZombie(const ZombieData &zombieData) {
    if(zombieManager.find(zombieData.zombieid) == zombieManager.end()) {
        createZombie(zombieData.zombieid);
    }
    Zombie& zombie = zombieManager[zombieData.zombieid].first;
    zombie.setPosition(zombieData.xpos, zombieData.ypos);
    zombie.setAngle(zombieData.direction);
    zombie.setHealth(zombieData.health);
}

/**
Date: 30. 17, 2017
Programmer: Brody McCrone and Deric Mccadden
Interface: void GameManager::handleAttackAction(const AttackAction& attackAction)
    attackAction: Information about an attack a marine performed received from
        the server.
Description:
-Doesn't update the players marine, because the player performs actions before
sending information to them about the server.
-If the marine exits, it fires its current weapon. Weapon id is in the attack
action but support for weapon ids hasn't been implemented so ignores it and
fires current weapon.
*/
void GameManager::handleAttackAction(const AttackAction& attackAction) {
    if (!(attackAction.playerid == player.getId())) {
        auto marine = marineManager[attackAction.playerid];
        if (marine.second) {
            int curX = marine.first.getX();
            int curY = marine.first.getY();
            double curAngle = marine.first.getAngle();
            marine.first.setPosition(attackAction.xpos, attackAction.ypos);
            marine.first.setAngle(attackAction.direction);
            marine.first.fireWeapon();
            marine.first.setPosition(curX, curY);
            marine.first.setAngle(curAngle);
        }
    }
}

/**
* Date: Mar. 1, 2017
* Modified: Mar. 15 2017 - Mark Tattrie
* Author: Terry
* Function Interface: int32_t GameManager::createBarricade(const float x, const float y)
* Description:
* Create barricade add it to manager, returns success
*/
int32_t GameManager::createBarricade(const float x, const float y) {
    const int32_t id = generateID();
    SDL_Rect temp = {INITVAL, INITVAL, DEFAULT_SIZE, DEFAULT_SIZE};

    SDL_Rect barricadeRect = temp;
    SDL_Rect moveRect = temp;
    SDL_Rect pickRect = temp;

    const auto& elem = barricadeManager.emplace(id, Barricade(id, barricadeRect, moveRect, pickRect));
    getBarricade(id).setSrcRect(WALL_SRC_X, WALL_SRC_Y, WALL_SRC_W, WALL_SRC_H);
    elem->second.setPosition(x,y);
    return id;
}

void GameManager::deleteBarricade(const int32_t id) {
    barricadeManager.erase(id);
#ifdef SERVER
    saveDeletion({UDPHeaders::BARRICADE, id});
#endif
}

// Get a barricade by its id
Barricade& GameManager::getBarricade(const int32_t id) {
    const auto& bar = barricadeManager[id];
    assert(bar.second);
    return bar.first;
}

/**
* Date: Mar. 14, 2017
* Modified: Mar. 15 2017 - Mark Tattrie
*           Mar. 16 2017 - Michael Goll
* Author: Maitiu Morton
* Function Interface: int32_t GameManager::createWall(const float x, const float y, const int w,
*       const int h)
* Description:
* Create wall, add it to manager, returns success
*/
int32_t GameManager::createWall(const float x, const float y, const int w, const int h) {
    const int32_t id = generateID();

    SDL_Rect wallRect = {static_cast<int>(x), static_cast<int>(y), w, h};
    SDL_Rect moveRect = {static_cast<int>(x), static_cast<int>(y), w, h};
    SDL_Rect pickRect = {static_cast<int>(x), static_cast<int>(y), w, h};

    wallManager.emplace(id, Wall(id, wallRect, moveRect, pickRect, h, h));
    return id;
}

/**
* Date: Mar. 1, 2017
* Modified: Mar. 8 2017 - Jacob McPhail
* Author: Terry Kang
* Function Interface: setBoundary(const float startX, const float startY,
*        const float endX, const float endY)
* Description:
*   Create test area.
*/
void GameManager::setBoundary(const float startX, const float startY, const float endX, const float endY) {
    int width = endX - startX + 200;
    int height = DEFAULT_SIZE;

    const float x = startX - DEFAULT_SIZE;
    const float y = startY - DEFAULT_SIZE;

    createWall(x, y, width, height);
    createWall(x, endY, width, height);

    width = DEFAULT_SIZE;
    height = endY - startY + DEFAULT_SIZE;

    createWall(endX, startY, width, height);
    createWall(x, startY, width, height);

    const float sX = (endX + startX) / 2 - BASE_WIDTH - DEFAULT_SIZE;
    const float eX = (endX + startX) / 2 + BASE_WIDTH + DEFAULT_SIZE;
    const float sY = (endY + startY) / 2 - BASE_HEIGHT - DEFAULT_SIZE;
    const float eY = (endY + startY) / 2 + BASE_HEIGHT + DEFAULT_SIZE;

    width = eX - sX;
    height = DEFAULT_SIZE;

    createWall(sX, sY, width / 2, height);
    createWall(sX + (width / 4 * 3), sY, width / 4, height);
    createWall(sX, eY, width / 4, height);
    createWall(sX + width / 2 + DEFAULT_SIZE, eY, width / 2, height);

    width = DEFAULT_SIZE;
    height = eY - sY;

    createWall(sX, sY, width, height / 2);
    createWall(sX, sY + (height / 4 * 3), width, height / 4);
    createWall(eX, sY, width, height / 1.5);
    createWall(eX, sY + (height / 4 * 3), width, height / 4);
}
