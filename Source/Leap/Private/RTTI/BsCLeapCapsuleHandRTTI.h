//************************************ bs::framework - Copyright 2018 Next Limit *****************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#pragma once

#include "Leap/BsCLeapCapsuleHand.h"
#include "Private/RTTI/BsGameObjectRTTI.h"

namespace bs
{
	/** @cond RTTI */
	/** @addtogroup RTTI-Impl-Leap
	 *  @{
	 */

	class CLeapCapsuleHandRTTI : public RTTIType<CLeapCapsuleHand, CLeapHandModelBase, CLeapCapsuleHandRTTI>
	{
	public:
		CLeapCapsuleHandRTTI()
		{ }

		void onDeserializationEnded(IReflectable* obj, const UnorderedMap<String, UINT64>& params) override
		{
			// Note: Since this is a CoreObject I should call initialize() right after deserialization,
			// but since this specific type is used in Components we delay initialization until Component
			// itself does it. Keep this is mind in case this ever needs to be deserialized for non-Component
			// purposes (you'll need to call initialize manually).
		}

		const String& getRTTIName() override
		{
			static String name = "CLeapCapsuleHand";
			return name;
		}

		UINT32 getRTTIId() override
		{
			return TID_CLeapCapsuleHand;
		}

		SPtr<IReflectable> newRTTIObject() override
		{
			return GameObjectRTTI::createGameObject<CLeapCapsuleHand>();
		}
	};

	/** @} */
	/** @endcond */
}