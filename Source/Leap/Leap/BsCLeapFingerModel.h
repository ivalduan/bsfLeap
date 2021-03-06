//************************************ bs::framework - Copyright 2018 Next Limit *****************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#pragma once

#include "Math/BsRay.h"
#include "Leap/BsLeapFrame.h"
#include "Scene/BsSceneObject.h"

namespace bs
{
	/** @addtogroup Leap
	*  @{
	*/

	/** The base component class for all finger models. */
	class CLeapFingerModel : public Component
	{
	public:
		CLeapFingerModel(const HSceneObject& parent);

		/** Returns the latest Leap hand data represented by this model. */
		const LeapHand* getLeapHand() const { return mHand; }

		/** Returns the latest Leap hand finger represented y this model. */
		const LeapFinger* getLeapFinger() const { return mFinger; }

		/** Sets the Leap hand and finger data for this finger. */
		void setLeapHand(const LeapHand* hand);

		/**
		* Implement this function to initialize this finger after it is created.
		* Typically, this function is called by the parent CLeapHandModel component.
		*/
		virtual void onInitModel();

		/**
		* Implement this function to update this finger once every game loop.
		* Typically, this function is called by the parent CLeapHandModel's updateFrame() function.
		*/
		virtual void updateFrame() = 0;

		/** Returns the location of the tip of the finger. */
		Vector3 getTipPosition();

		/** Returns the location of the given joint on the finger. */
		Vector3 getJointPosition(int joint);

		/** Returns a ray from the tip of the finger in the direction it is pointing. */
		Ray getRay();

		/** Returns the center of the given bone on the finger. */
		Vector3 getBoneCenter(int bone);

		/** Returns the direction the given bone is facing on the finger. */
		Vector3 getBoneDirection(int bone);

		/** Returns the rotation quaternion of the given bone. */
		Quaternion getBoneRotation(int bone);

		/** Returns the length of the finger bone. */
		float getBoneLength(int bone);

		/** Returns the width of the finger bone. */
		float getBoneWidth(int bone);

		/**
		 * Returns Mecanim stretch angle in the range (-180, +180]
		 * Positive stretch opens the hand. For the thumb this moves it away from the palm.
		 */
		float getFingerJointStretchMecanim(int joint);

		/**
		 * Returns Mecanim spread angle, which only applies to joint_type = 0
		 * Positive spread is towards thumb for index and middle, but is in the opposite direction for the ring and pinky.
		 * For the thumb negative spread rotates the thumb in to the palm.
		 */
		float getFingerJointSpreadMecanim();

	public:
		/** The number of bones in a finger. */
		static constexpr int NUM_BONES = 4;

		/** The number of joints in a finger. */
		static constexpr int NUM_JOINTS = 3;

		LeapFinger::Type mType = LeapFinger::TYPE_INDEX;

		/** Bone SceneObjects positioned and rotated by this model. */
		HSceneObject mBones[NUM_BONES];

		/** Joint SceneObjects positioned and rotated by this model. */
		HSceneObject mJoints[NUM_BONES - 1];

	protected:
		/** Latest Leap hand data. */
		const LeapHand* mHand;

		/** Latest Leap finger data. */
		const LeapFinger* mFinger;

		/************************************************************************/
		/* 								RTTI		                     		*/
		/************************************************************************/
	public:
		friend class CLeapFingerModelRTTI;
		static RTTITypeBase* getRTTIStatic();
		RTTITypeBase* getRTTI() const override;

	protected:
		CLeapFingerModel();// Serialization only
	};

	/** @} */
}