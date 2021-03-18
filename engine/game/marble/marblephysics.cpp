//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

static U32 sCollisionMask = StaticObjectType |
                            AtlasObjectType |
                            InteriorMapObjectType |
                            ShapeBaseObjectType |
                            PlayerObjectType |
                            VehicleBlockerObjectType;

static U32 sContactMask = StaticObjectType |
                          AtlasObjectType |
                          InteriorMapObjectType |
                          ShapeBaseObjectType |
                          PlayerObjectType |
                          VehicleBlockerObjectType;

bool gMarbleAxisSet = false;
Point3F gWorkGravityDir;
Point3F gMarbleSideDir;

Point3D Marble::getVelocityD() const
{
    return mVelocity;
}

void Marble::setVelocityD(const Point3D& vel)
{
    dMemcpy(mVelocity, vel, sizeof(mVelocity));
    mSinglePrecision.mVelocity = vel;

    setMaskBits(MoveMask);
}

void Marble::setVelocityRotD(const Point3D& rot)
{
    dMemcpy(mOmega, rot, sizeof(mOmega));

    setMaskBits(MoveMask);
}

void Marble::applyImpulse(const Point3F& pos, const Point3F& vec)
{
    // TODO: Implement applyImpulse
    Parent::applyImpulse(pos, vec);
}

void Marble::clearMarbleAxis()
{
    gMarbleAxisSet = false;
    mGravityFrame.mulP(Point3F(0.0f, 0.0f, -1.0f), &gWorkGravityDir);
}

void Marble::applyContactForces(const Move* move, bool isCentered, Point3D& aControl, const Point3D& desiredOmega, F64 timeStep, Point3D& A, Point3D& a, F32& slipAmount)
{
    F32 force = 0.0f;
    S32 bestContactIndex = mContacts.size();
    for (S32 i = 0; i < mContacts.size(); i++)
    {
        Contact* contact = &mContacts[i];

        SimObject* obj = contact->object;
        if (!obj || (obj->getType() & PlayerObjectType) == 0)
        {
            contact->normalForce = -mDot(contact->normal, A);

            if (contact->normalForce > force)
            {
                force = contact->normalForce;
                bestContactIndex = i;
            }
        }

    }

    if (bestContactIndex != mContacts.size() && (mMode & MoveMode) != 0)
        dMemcpy(&mBestContact, &mContacts[bestContactIndex], sizeof(mBestContact));

    if (move->trigger[2] && bestContactIndex != mContacts.size())
    {
        F64 friction = (mVelocity.z - mBestContact.surfaceVelocity.z) * mBestContact.normal.z
                     + (mVelocity.y - mBestContact.surfaceVelocity.y) * mBestContact.normal.y
                     + (mVelocity.x - mBestContact.surfaceVelocity.x) * mBestContact.normal.x;

        if (mDataBlock->jumpImpulse > friction)
        {
            mVelocity += (mDataBlock->jumpImpulse - friction) * mBestContact.normal;

            if (mDataBlock->sound[MarbleData::Jump])
            {
                if (isGhost())
                {
                    MatrixF mat(true);
                    mat.setColumn(3, getPosition());
                    alxPlay(mDataBlock->sound[MarbleData::Jump], &mat);
                }
            }
        }
    }

    for (S32 i = 0; i < mContacts.size(); i++)
    {
        Contact* contact = &mContacts[i];

        F64 normalForce = -mDot(contact->normal, A);

        if (normalForce > 0.0 &&
              (mVelocity.x - contact->surfaceVelocity.x) * contact->normal.x
            + (mVelocity.y - contact->surfaceVelocity.y) * contact->normal.y
            + (mVelocity.z - contact->surfaceVelocity.z) * contact->normal.z <= 0.0001)
        {
            A += contact->normal * normalForce;
        }
    }

    if (bestContactIndex != mContacts.size() && (mMode & MoveMode) != 0)
    {
        Point3D aadd = -mBestContact.normal * mRadius;

        Point3D aFriction;
        mCross(mOmega, aadd, &aFriction);

        Point3D vAtC = aFriction + mVelocity - mBestContact.surfaceVelocity;
        mBestContact.vAtCMag = vAtC.len();

        Point3D aNewFriction(0, 0, 0);
        Point3D ANewFriction(0, 0, 0);

        if (mBestContact.vAtCMag == 0.0)
            goto LABEL_34;

        bool slipping = true;

        F32 frictiona = 0.0f;
        if ((mMode & RestrictXYZMode) == 0)
            frictiona = mDataBlock->kineticFriction * mBestContact.friction;

        F64 somevar = frictiona * 5.0 * mBestContact.normalForce / (mRadius + mRadius);
        F64 AMagnitude = mBestContact.normalForce * frictiona;
        F64 totalDeltaV = (mRadius * somevar + AMagnitude) * timeStep;
        if (mBestContact.vAtCMag < totalDeltaV)
        {
            slipping = false;
            somevar *= mBestContact.vAtCMag / totalDeltaV;
            AMagnitude *= mBestContact.vAtCMag / totalDeltaV;
        }

        Point3D vAtCDir = vAtC * (1.0 / mBestContact.vAtCMag);
        Point3D invVAtCDir = -vAtCDir;

        Point3D aAtC;
        mCross(-mBestContact.normal, invVAtCDir, &aAtC);

        aNewFriction = aAtC * somevar;
        ANewFriction = vAtCDir * -AMagnitude;

        slipAmount = mBestContact.vAtCMag - totalDeltaV;

        if (!slipping)
        {
LABEL_34:
            Point3D invGrav = -gWorkGravityDir;

            Point3D wow = invGrav * mRadius;

            Point3D poptart;
            mCross(wow, A, &poptart);

            Point3D newThing = poptart * (1.0 / wow.lenSquared());

            if (isCentered)
            {
                aControl = desiredOmega - (a * timeStep + mOmega);

                if (mDataBlock->brakingAcceleration < aControl.len())
                    aControl *= mDataBlock->brakingAcceleration / aControl.len();
            }

            Point3D invNormRad = -mBestContact.normal * mRadius;

            Point3D res;
            mCross(aControl, invNormRad, &res);
            res = -res;

            Point3D res2;
            mCross(newThing, invNormRad, &res2);
            res2 += res;

            F64 mag = res2.len();

            F64 theFriction = 0.0;
            if ((mMode & RestrictXYZMode) == 0)
                theFriction = mDataBlock->staticFriction * mBestContact.friction;

            if (theFriction * mBestContact.normalForce < mag)
            {
                theFriction = 0.0;
                if ((mMode & RestrictXYZMode) == 0)
                    theFriction = mDataBlock->kineticFriction * mBestContact.friction;

                res *= theFriction * mBestContact.normalForce / mag;
            }

            A += res;
            a += newThing;
        }

        A += ANewFriction;
        a += aNewFriction;
    }

    a += aControl;
}

void Marble::getMarbleAxis(Point3D& sideDir, Point3D& motionDir, Point3D& upDir)
{
    if (!gMarbleAxisSet)
    {
        MatrixF camMat;
        mGravityFrame.setMatrix(&camMat);


        MatrixF xRot;
        m_matF_set_euler(Point3F(mMouseY, 0, 0), xRot);

        MatrixF zRot;
        m_matF_set_euler(Point3F(0, 0, mMouseX), zRot);

        camMat.mul(zRot);
        camMat.mul(xRot);

        gMarbleMotionDir.x = camMat[1];
        gMarbleMotionDir.y = camMat[5];
        gMarbleMotionDir.z = camMat[9];

        mCross(gMarbleMotionDir, -gWorkGravityDir, gMarbleSideDir);
        m_point3F_normalize(&gMarbleSideDir.x);
        
        mCross(-gWorkGravityDir, gMarbleSideDir, gMarbleMotionDir);
        
        gMarbleAxisSet = 1;
    }

    sideDir = gMarbleSideDir;
    motionDir = gMarbleMotionDir;
    upDir = -gWorkGravityDir;
}

const Point3F& Marble::getMotionDir()
{
    Point3D side;
    Point3D motion;
    Point3D up;
    Marble::getMarbleAxis(side, motion, up);

    return gMarbleMotionDir;
}

bool Marble::computeMoveForces(Point3D& aControl, Point3D& desiredOmega, const Move* move)
{
    aControl.set(0, 0, 0);
    desiredOmega.set(0, 0, 0);

    Point3F invGrav = -gWorkGravityDir;

    Point3D r = invGrav * mRadius;

    Point3D rollVelocity;
    mCross(mOmega, r, rollVelocity);

    Point3D sideDir;
    Point3D motionDir;
    Point3D upDir;
    getMarbleAxis(sideDir, motionDir, upDir);

    Point2F currentVelocity(mDot(sideDir, rollVelocity), mDot(motionDir, rollVelocity));

    Point2F mv(move->x, move->y);
    mv *= 1.538461565971375;

    if (mv.len() > 1.0f)
        m_point2F_normalize_f(mv, 1.0f);

    Point2F desiredVelocity = mv * mDataBlock->maxRollVelocity;

    if (desiredVelocity.x == 0.0f && desiredVelocity.y == 0.0f)
        return 1;

    // TODO: Clean up gotos

    float cY;

    float desiredYVel = desiredVelocity.y;
    float currentYVel = currentVelocity.y;
    float desiredVelX;
    if (currentVelocity.y > desiredVelocity.y)
    {
        cY = currentVelocity.y;
        if (desiredVelocity.y > 0.0f)
        {
LABEL_11:
            desiredVelX = desiredVelocity.x;
            desiredVelocity.y = cY;
            goto LABEL_13;
        }
        currentYVel = currentVelocity.y;
        desiredYVel = desiredVelocity.y;
    }

    if (currentYVel < desiredYVel)
    {
        cY = currentYVel;
        if (desiredYVel < 0.0f)
            goto LABEL_11;
    }
    desiredVelX = desiredVelocity.x;
LABEL_13:
    float cX = currentVelocity.x;
    float v17;
    if (currentVelocity.x > desiredVelX)
    {
        if (desiredVelX > 0.0f)
        {
            v17 = currentVelocity.x;
LABEL_16:
            desiredVelocity.x = v17;
            goto LABEL_20;
        }
        cX = currentVelocity.x;
    }

    if (cX < desiredVelX)
    {
        v17 = cX;
        if (desiredVelX < 0.0f)
            goto LABEL_16;
    }

LABEL_20:
    Point3D newMotionDir = sideDir * desiredVelocity.x + motionDir * desiredVelocity.y;

    Point3D newSideDir;
    mCross(r, newMotionDir, newSideDir);

    desiredOmega = newSideDir * (1.0f / r.lenSquared());
    aControl = desiredOmega - mOmega;

    // Prevent increasing marble speed with diagonal movement
    if (mDataBlock->angularAcceleration < aControl.len())
        aControl *= mDataBlock->angularAcceleration / aControl.len();

    return false;
}

void Marble::velocityCancel(bool surfaceSlide, bool noBounce, bool& bouncedYet, bool& stoppedPaths, Vector<PathedInterior*>& pitrVec)
{
    U32 itersIn = 0;
    bool looped = false;
    bool done;

    do
    {
        ++itersIn;
        done = true;

        if (!mContacts.empty())
        {
            S32 i = 0;
            do
            {
                Contact* contact =  &mContacts[i];

                Point3D velDiff = mVelocity - contact->surfaceVelocity;
                F64 velDiffDot = mDot(contact->normal, velDiff);
                if ((looped || velDiffDot >= 0.0) && velDiffDot >= -0.0001)
                    goto LABEL_27;

                F64 velLen = mVelocity.len();

                Point3D normVel = contact->normal * velDiffDot;
                if (isGhost() && !bouncedYet)
                {
                    playBounceSound(*contact, -velDiffDot);
                    bouncedYet = true;
                }

                if (noBounce)
                    goto LABEL_11;

                if (contact->object == NULL || (contact->object->getType() & PlayerObjectType) == 0)
                {
                    if (contact->surfaceVelocity.len() != 0.0 || surfaceSlide || -mDataBlock->maxDotSlide * velLen >= velDiffDot)
                    {
                        if (-mDataBlock->minBounceVel < velDiffDot)
                        {
LABEL_11:
                            mVelocity -= normVel;
                        } else
                        {
                            F32 bounceFactor = contact->restitution * mPowerUpParams.bounce;
                            F32 invertedBounceFactor = -(bounceFactor + 1.0f);

                            Point3D velocityAdd = normVel * invertedBounceFactor;

                            Point3D mul = -contact->normal * mRadius;

                            Point3D marbleBox;

                            mCross(mOmega, mul, &marbleBox);

                            Point3D contactNormal = contact->normal;

                            Point3D vAtC = marbleBox + velDiff;

                            F64 normalContactVelocity = -mDot(contactNormal, velDiff);

                            F64 velDiffLength = velDiff.len();

                            F64 velDiffBounceFactor = velDiffLength * bounceFactor;
                            bounceEmitter(velDiffBounceFactor, contactNormal);

                            F64 normalYVelDiff = mDot(contactNormal, velDiff);

                            vAtC -= contactNormal * normalYVelDiff;

                            F64 vAtCLen = vAtC.len();
                            if (vAtCLen != 0.0) {
                                F64 friction = mDataBlock->bounceKineticFriction * contact->friction;

                                F64 frictionContactVelocity = friction * 5.0 * normalContactVelocity / (mRadius + mRadius);
                                if (vAtCLen / mRadius < frictionContactVelocity) {
                                    frictionContactVelocity = vAtCLen / mRadius;
                                }
                                Point3D marbleBox2 = -(vAtC * (1.0 / vAtCLen));
                                Point3D invNormal = -contact->normal;
                                Point3D rotation;

                                mCross(invNormal, marbleBox2, &rotation);

                                mOmega += rotation * frictionContactVelocity;

                                Point3D marbleBox3 = -contactNormal * mRadius;
                                Point3D negFrictionContactVelocity = -(rotation * frictionContactVelocity);

                                Point3D point3D1;
                                mCross(negFrictionContactVelocity, marbleBox3, &point3D1);
                                mVelocity -= point3D1;
                            }
                           
                            mVelocity += velocityAdd;
                        }
                        goto LABEL_26;
                    }
                    mVelocity -= normVel;
                    m_point3D_normalize(mVelocity);
                    mVelocity *= velLen;
                    surfaceSlide = true;

                } else
                {
                    F64 mass = ((SceneObject*)contact->object)->getMass();
                    // TODO: Implement velocityCancel
                }
LABEL_26:
                done = false;
LABEL_27:
                ++i;
            } while(i < mContacts.size());
        }

        looped = true;

        if (itersIn > 6 && !stoppedPaths)
        {
            stoppedPaths = true;
            if (noBounce)
                done = true;

            for (S32 j = 0; j < mContacts.size(); j++)
            {
                mContacts[j].surfaceVelocity.set(0, 0, 0);
            }

            for (S32 k = 0; k < pitrVec.size(); k++)
            {
                PathedInterior* pitr = pitrVec[k];

                if (pitr->getExtrudedBox().isOverlapped(mWorldBox))
                    pitr->setStopped();
            }
        }


    } while(!done && itersIn < 20);

    if (mVelocity.lenSquared() < 625.0)
    {
        // TODO: Implement velocityCancel
        
    }
    
}

Point3D Marble::getExternalForces(const Move* move, F64 timeStep)
{
    if ((mMode & MoveMode) == 0)
        return mVelocity * -16.0;

    Point3D ret = gWorkGravityDir * mDataBlock->gravity * mPowerUpParams.gravityMod;
    //Point3D ret(0, 0, 0); // <- to disable gravity when testing

    // TODO: Finish Implementing getExternalForces

    if (mContacts.size() == 0 && (mMode & RestrictXYZMode) == 0)
    {
        Point3D sideDir;
        Point3D motionDir;
        Point3D upDir;
        getMarbleAxis(sideDir, motionDir, upDir);

        Point3F movement = sideDir * move->x + motionDir * move->y;
        ret += movement * mPowerUpParams.airAccel;
    }

    return ret;
}

void Marble::advancePhysics(const Move* move, U32 timeDelta)
{
    dMemcpy(&delta.posVec, &mPosition, sizeof(delta.posVec));

    smPathItrVec.clear();

    F32 dt = timeDelta / 1000.0;

    Box3F extrudedMarble = this->mWorldBox;

    Point3F velocityExpansion = (mVelocity * dt) * 1.100000023841858;
    Point3F absVelocityExpansion = velocityExpansion.abs();

    extrudedMarble.min += (velocityExpansion - absVelocityExpansion) * 0.5f;
    extrudedMarble.max += (velocityExpansion + absVelocityExpansion) * 0.5f;

    extrudedMarble.min -= dt * 25.0;
    extrudedMarble.max += dt * 25.0;

    for (PathedInterior* obj = PathedInterior::getPathedInteriors(this); ; obj = obj->getNext())
    {
        if (!obj)
            break;

        if (extrudedMarble.isOverlapped(obj->getExtrudedBox()))
        {
            obj->pushTickState();
            obj->computeNextPathStep(timeDelta);
            smPathItrVec.push_back(obj);
        }
    }

    resetObjectsAndPolys(sContactMask, extrudedMarble);

    bool bouncedYet = false;
    
    mMovePathSize = 0;
    
    F64 timeRemaining = timeDelta / 1000.0;
    F64 startTime = timeRemaining;
    F32 slipAmount = 0.0;
    F64 contactTime = 0.0;

    U32 it = 0;
    do
    {
        if (timeRemaining == 0.0)
            break;

        F64 timeStep = 0.00800000037997961;
        if (timeRemaining < 0.00800000037997961)
            timeStep = timeRemaining;

        Point3D aControl;
        Point3D desiredOmega;

        bool isCentered = computeMoveForces(aControl, desiredOmega, move);

        findContacts(sContactMask, NULL, NULL);

        bool stoppedPaths;
        velocityCancel(isCentered, false, bouncedYet, stoppedPaths, smPathItrVec);
        Point3D A = getExternalForces(move, timeStep);

        Point3D a(0, 0, 0);
        applyContactForces(move, isCentered, aControl, desiredOmega, timeStep, A, a, slipAmount);

        mVelocity += A * timeStep;
        mOmega += a * timeStep;

        if ((mMode & RestrictXYZMode) != 0)
            mVelocity.set(0, 0, 0);

        velocityCancel(isCentered, true, bouncedYet, stoppedPaths, smPathItrVec);

        F64 moveTime = timeStep;
        computeFirstPlatformIntersect(moveTime, smPathItrVec);
        testMove(mVelocity, mPosition, moveTime, mRadius, sCollisionMask, false);
        //mPosition += mVelocity * moveTime;

        if (!mMovePathSize && timeStep * 0.99 > moveTime && moveTime > 0.001000000047497451)
        {
            F64 diff = startTime - timeRemaining;

            mMovePath[0] = mPosition;
            mMovePathTime[mMovePathSize] = (diff + moveTime) / startTime;

            mMovePathSize++;
        }

        F64 currentTimeStep = timeStep;
        if (timeStep != moveTime)
        {
            F64 diff = timeStep - moveTime;

            mVelocity -= A * diff;
            mOmega -= a * diff;

            currentTimeStep = moveTime;
        }

        if (!mContacts.empty())
            contactTime += currentTimeStep;

        timeRemaining -= currentTimeStep;

        timeStep = (startTime - timeRemaining) * 1000.0;

        for (S32 i = 0; i < smPathItrVec.size(); i++)
        {
            PathedInterior* pint = smPathItrVec[i];
            pint->resetTickState(0);
            pint->advance(timeStep);
        }

        it++;
    } while (it <= 10);

    for (S32 i = 0; i < smPathItrVec.size(); i++)
        smPathItrVec[i]->popTickState();

    F32 contactPct = contactTime * 1000.0 / timeDelta;

    Con::setFloatVariable("testCount", contactPct);
    Con::setFloatVariable("marblePitch", mMouseY);

    updateRollSound(contactPct, slipAmount);

    dMemcpy(&delta.pos, &mPosition, sizeof(delta.pos));

    delta.posVec -= delta.pos;

    setPosition(mPosition, 0);
}

ConsoleMethod(Marble, setVelocityRot, bool, 3, 3, "(vel)")
{
    Point3F rot;
    dSscanf(argv[2], "%f %f %f", &rot.x, &rot.y, &rot.z);
    object->setVelocityRotD(rot);

    return 1;
}
