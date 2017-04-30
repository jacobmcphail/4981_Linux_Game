#include "Marine.h"
#include <cstdlib>
#include "../game/GameManager.h"
#include "../log/log.h"

/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: Marine(const int32_t id, const SDL_Rect& dest,
 *              const SDL_Rect& movementSize, const SDL_Rect& projectileSize, const SDL_Rect& damageSize)
 *
 *              id : Marine id
 *              dest : Destination rect
 *              movmentSize : Move hitbox size
 *              projectileSize : Projectile hitbox size
 *              damageSize : Damage hitbox size
 *
 * Description:
 *     ctor for a marine.
 */
Marine::Marine(const int32_t id, const SDL_Rect& dest, const SDL_Rect& movementSize,
        const SDL_Rect& projectileSize, const SDL_Rect& damageSize)
: Entity(id, dest, movementSize, projectileSize, damageSize),
        Movable(id, dest, movementSize, projectileSize, damageSize, MARINE_VELOCITY), atStore(false){
    logv("Create Marine\n");
}

/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: ~Marine()
 * Description:
 *   dctor for a marine.
 */
Marine::~Marine() {
    logv("Destroy Marine\n");
}

/**
 * Date: Feb. 4, 2017
 * Modified: ----
 * Author: Jacob McPhail
 * Function Interface: onCollision()
 */
void Marine::onCollision() {
    // Do nothing for now
}

/**
 * Date: Feb. 4, 2017
 * Modified:
 * Apr. 10, 2017, Alex Zielinski
 * Author: Jacob McPhail, Alex Zielinski
 * Function Interface: collidingProjectile(const int damage)
 *      damage : damage to deal to marine
 *
 * Description:
 *     Deals damage to a marine.
 * 
 * Revisions:
 * Apr.10 , 2017, Alex Zielinski - implemented zombie attack sound effect
 */
void Marine::collidingProjectile(const int damage) {
    // play zombie attack sound effect
    AudioManager::instance().playEffect(EFX_ZATTACK01);
    health -= damage;
}

// Created by DericM 3/8/2017
bool Marine::fireWeapon() {
    Weapon *w = inventory.getCurrent();
    if(w) {
        return w->fire(*this);
    } else {
        logv("Slot Empty\n");
        return false;
    }
}


/*
 * Created By Maitiu
 * Modified: Mar. 15 2017 - Mark Tattrie
 * Description: Checks The pick up Hitboxes of the Weapon Drops and Turrets to see if the player's
 * Marine is touching them IF Touching a Weapon Drop it Calls the Inventory Pick up method.
 *
 * Modified:
 * Apr. 10, 2017, Mark Chen - Adjusted to check for turrets with new Tree-Entity system
 */
int32_t Marine::checkForPickUp() {
    int32_t pickId = -1;
    GameManager *gm = GameManager::instance();
    CollisionHandler& ch = gm->getCollisionHandler();

    Entity *ep = ch.detectPickUpCollision(ch.getQuadTreeEntities(ch.getStoreTree(),this),this);
    if(ep){
        activateStore(ep);
        return -1;
    }

    // checks if Id matches any turret Ids in turretManager, if yes, then return with the Id
    ep = ch.detectPickUpCollision(ch.getQuadTreeEntities(ch.getTurretTree(),this),this);
    if(ep) {
        pickId = ep->getId();
        if (gm->getTurretManager().count(pickId)) {
            return pickId;
        }
    }

    ep = ch.detectPickUpCollision(ch.getQuadTreeEntities(ch.getPickUpTree(),this),this);
    if(ep) {
        //get Entity drop Id
        pickId = ep->getId();
        logv("Searching for id:%d in weaponDropManager\n", pickId);
        if(gm->weaponDropExists(pickId)) {
            const WeaponDrop& wd = gm->getWeaponDrop(pickId);
            //Get Weaopn id from weapon drop
            pickId = wd.getWeaponId();
            //Picks up Weapon
            if(inventory.pickUp(pickId, wd.getX(), wd.getY())) {
                int32_t dropPoint = wd.getDropPoint();
                if(dropPoint != -1){
                    gm->freeDropPoint(dropPoint);
                }
                gm->deleteWeaponDrop(wd.getId());
            }
        } else if(gm->barricadeDropExists(pickId)) {//check if a barricade drop
            int32_t dropPoint = gm->getBarricadeDrop(pickId).getDropPoint();
            if(dropPoint != -1){
                gm->freeDropPoint(dropPoint);
            }
            gm->deleteBarricadeDrop(pickId);
            gm->getPlayer().handleTempBarricade(Renderer::instance().getRenderer());
        } else if(gm->consumeDropExists(pickId)) {//check if  consumdrop
            int32_t dropPoint = gm->getConsumeDrop(pickId).getDropPoint();
            inventory.pickUpConsumable(gm->getConsumeDrop(pickId).getConsumeId());
            if(dropPoint != -1){
                gm->freeDropPoint(dropPoint);
            }
            gm->deleteConsumeDrop(pickId);
        }
    } else {
        loge("Pick id was nullptr\n");
    }
    return -1;
}

/**
* Date: Mar 27
* Author: Aing Ragunathan
*
* Interface: void Marine::updateImageDirection()
*
* Description:
*       This function changes the direction that the character is facing.
*       It is called from GameStateMatch::handle after every frame and
*       updates the direction of the sprite according to the angle of the
*       mouse from the center of the screen.
*/
void Marine::updateImageDirection() {
    const double radians = getRadianAngle() - M_PI / 2;

    //order: start from ~0 rad, counter clockwise
    if (radians > SPRITE_ANGLE2 && radians < SPRITE_ANGLE1) {
        setSrcRect(getSrcRect().x, SPRITE_RIGHT, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    } else if (radians > SPRITE_ANGLE3 && radians < SPRITE_ANGLE2) {
        setSrcRect(getSrcRect().x, SPRITE_BACK_RIGHT, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    } else if (radians > SPRITE_ANGLE4 && radians < SPRITE_ANGLE3) {
        setSrcRect(getSrcRect().x, SPRITE_BACK, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    } else if (radians > SPRITE_ANGLE5 && radians < SPRITE_ANGLE4) {
        setSrcRect(getSrcRect().x, SPRITE_BACK_LEFT, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    } else if (radians > SPRITE_ANGLE6 && radians < SPRITE_ANGLE5) {
        setSrcRect(getSrcRect().x, SPRITE_LEFT, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    } else if (radians > SPRITE_ANGLE7 && radians < SPRITE_ANGLE6) {
        setSrcRect(getSrcRect().x, SPRITE_FRONT_LEFT, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    } else if (radians < SPRITE_ANGLE7) {
        setSrcRect(getSrcRect().x, SPRITE_FRONT, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    } else if (radians > SPRITE_ANGLE1 && radians < SPRITE_ANGLE8) {
        setSrcRect(getSrcRect().x, SPRITE_FRONT_RIGHT, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    }
}

/**
* Date: Mar 30
* Modified: April 4 (Brody McCrone) - Swapped use of keyboard input to deltas.
*
* Author: Aing Ragunathan
*
* Function Interface: void Marine::updateImageWalk(double frameCount)
*       double frameCount - counted frames while walking
*
* Description:
*       This function repeatedly updates the image of the marine in order to animate
*       walking. It is called from GameManager::updateMarines every frame.
*/
void Marine::updateImageWalk() {
    static unsigned long frameCount = 0;
    ++frameCount;
    const float dy = getDY();
    const float dx = getDX();
    if (dy > 0) {
        //stops lag when taking first step
        if (getSrcRect().x == SPRITE_FRONT) {
            setSrcRect(SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
        } else if (frameCount % FRAME_COUNT_WALK == 0) {
            //cycle throught the walking images
            if (getSrcRect().x < SPRITE_NEXT_STEP) {
                setSrcRect(getSrcRect().x + SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
            } else {
                setSrcRect(SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
            }
        }
    } else if (dy < 0) {
        if (getSrcRect().x == SPRITE_FRONT) {
            setSrcRect(SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
        } else if (frameCount % FRAME_COUNT_WALK == 0) {
            if (getSrcRect().x < SPRITE_NEXT_STEP) {
                setSrcRect(getSrcRect().x + SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
            } else {
                setSrcRect(SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
            }
        }
    } else if (dx < 0) {
        if (getSrcRect().x == SPRITE_FRONT) {
            setSrcRect(SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
        } else if (frameCount % FRAME_COUNT_WALK == 0) {
            if (getSrcRect().x < SPRITE_NEXT_STEP) {
                setSrcRect(getSrcRect().x + SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
            } else {
                setSrcRect(SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
            }
        }
    } else if (dx > 0) {
        if (getSrcRect().x == SPRITE_FRONT) {
            setSrcRect(SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
        } else if (frameCount % FRAME_COUNT_WALK == 0) {
            if (getSrcRect().x < SPRITE_NEXT_STEP) {
                setSrcRect(getSrcRect().x +SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
            } else {
                setSrcRect(SPRITE_SIZE_X, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
            }
        }
    } else {
        setSrcRect(SPRITE_FRONT, getSrcRect().y, SPRITE_SIZE_X, SPRITE_SIZE_Y);
    }
}

/*
 *Create by Maitiu March 30
 * Takes in an Entity that is a store and attempts a purchase
 */
void Marine::activateStore(const Entity *ep){
    GameManager *gm = GameManager::instance();
    if(gm->storeExists(ep->getId())){
        enterStore();
        gm->getStore(ep->getId())->openStore();
    }
}
