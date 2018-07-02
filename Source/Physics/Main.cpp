// Framework includes
#include "BsApplication.h"
#include "Components/BsCCamera.h"
#include "Components/BsCRenderable.h"
#include "Components/BsCSkybox.h"
#include "Components/BsCPlaneCollider.h"
#include "Components/BsCBoxCollider.h"
#include "Components/BsCCapsuleCollider.h"
#include "Components/BsCSphereCollider.h"
#include "Components/BsCCharacterController.h"
#include "Components/BsCRigidbody.h"
#include "GUI/BsCGUIWidget.h"
#include "GUI/BsGUIPanel.h"
#include "GUI/BsGUILayoutY.h"
#include "GUI/BsGUILabel.h"
#include "Input/BsInput.h"
#include "Material/BsMaterial.h"
#include "Physics/BsPhysicsMaterial.h"
#include "Platform/BsCursor.h"
#include "RenderAPI/BsRenderAPI.h"
#include "RenderAPI/BsRenderWindow.h"
#include "RenderAPI/BsVertexDataDesc.h"
#include "Renderer/BsRendererMeshData.h"
#include "Resources/BsResources.h"
#include "Resources/BsBuiltinResources.h"
#include "Scene/BsSceneObject.h"
#include "Utility/BsShapeMeshes3D.h"

// Example includes
#include "BsExampleFramework.h"
#include "BsFPSCamera.h"
#include "BsFPSWalker.h"
#include "Leap/BsCLeapCapsuleHand.h"
#include "Leap/BsCLeapHandEnableDisable.h"
#include "Leap/BsCLeapHandModelManager.h"
#include "Leap/BsCLeapRigidFinger.h"
#include "Leap/BsCLeapRigidHand.h"
#include "Leap/BsLeapService.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This example sets up a physical environment in which the user can walk around using the character controller component,
// and shoot the placed geometry demonstrating various aspects of the physics system. This includes a demonstration of
// static colliders, dynamic rigidbodies, physical materials, character controller and manual application of forces.
//
// The example first loads necessary resources, including textures, materials, textures and physical materials.. Then it
// set up the scene, consisting of a floor, and multiple stacks of boxes that can be knocked down. Character controller is
// created next, as well as the camera. Components for moving the character controller and the camera are attached to
// allow the user to control the character. Finally an input callback is hooked up that shoots spheres when user presses
// the left mouse button. 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace bs
{
	constexpr float GROUND_PLANE_SCALE = 50.0f;

	constexpr float HAND_SPHERE_RADIUS = 0.1f;

	UINT32 windowResWidth = 1280;
	UINT32 windowResHeight = 720;

	Vector3 leapProviderPos(0.0f, -1.0f, 1.0f);
	Vector3 leapProviderScl(0.01f, 0.01f, 0.01f);

	struct HandAssets
	{
		HMesh sphereMesh;
		HMesh cylinderMesh;
	};

	HandAssets assets;

	/** Load the resources we'll be using throughout the example. */
	void loadHandAssets()
	{
		// Generate 3D sphere model

		SPtr<VertexDataDesc> vertexDesc = bs_shared_ptr_new<VertexDataDesc>();
		vertexDesc->addVertElem(VET_FLOAT3, VES_POSITION);
		vertexDesc->addVertElem(VET_FLOAT2, VES_TEXCOORD);
		vertexDesc->addVertElem(VET_FLOAT3, VES_NORMAL);
		vertexDesc->addVertElem(VET_FLOAT4, VES_TANGENT);
		vertexDesc->addVertElem(VET_COLOR, VES_COLOR);

		UINT32 sphereNumVertices = 0;
		UINT32 sphereNumIndices = 0;
		ShapeMeshes3D::getNumElementsSphere(3, sphereNumVertices, sphereNumIndices);
		SPtr<MeshData> sphereMeshData = bs_shared_ptr_new<MeshData>(sphereNumVertices, sphereNumIndices, vertexDesc);

		ShapeMeshes3D::solidSphere(Sphere(Vector3::ZERO, HAND_SPHERE_RADIUS), sphereMeshData, 0, 0, 3);

		assets.sphereMesh = Mesh::create(sphereNumVertices, sphereNumIndices, vertexDesc);
		assets.sphereMesh->writeData(sphereMeshData, true);

		// Generate 3D cylinder model

		UINT32 cylinderNumVertices = 0;
		UINT32 cylinderNumIndices = 0;
		ShapeMeshes3D::getNumElementsCylinder(3, cylinderNumVertices, cylinderNumIndices);
		SPtr<MeshData> cylinderMeshData = bs_shared_ptr_new<MeshData>(cylinderNumVertices, cylinderNumIndices, vertexDesc);

		ShapeMeshes3D::solidCylinder(Vector3::ZERO, Vector3::UNIT_X, 0.3f, 0.1f, Vector2(1.0f, 1.0f), cylinderMeshData, 0, 0, 3);

		assets.cylinderMesh = Mesh::create(cylinderNumVertices, cylinderNumIndices, vertexDesc);
		assets.cylinderMesh->writeData(cylinderMeshData, true);
	}

	void setUpCapsuleHand(HSceneObject handsSO, eLeapHandType chirality)
	{
		String suffix = (chirality == eLeapHandType_Left) ? "_L" : "_R";

		HSceneObject handSO = SceneObject::create("CapsuleHand" + suffix);

		HLeapCapsuleHand handModel = handSO->addComponent<CLeapCapsuleHand>();
		handModel->mChirality = chirality;

		HLeapHandEnableDisable handTransition = handSO->addComponent<CLeapHandEnableDisable>();

		HSceneObject palmSO = SceneObject::create("palm");

		HRenderable palmRenderable = palmSO->addComponent<CRenderable>();
		palmRenderable->setMesh(assets.sphereMesh);

		palmSO->setParent(handSO);

		for (UINT32 f = 0; f < 5; ++f)
		{
			HSceneObject fingerSO = SceneObject::create("finger" + toString(f));

			HRenderable fingerRenderable = fingerSO->addComponent<CRenderable>();
			fingerRenderable->setMesh(assets.sphereMesh);

			fingerSO->setParent(handSO);

			for (UINT32 b = 0; b < 3; ++b)
			{
				HSceneObject boneSO = SceneObject::create("bone" + toString(b));

				HRenderable boneRenderable = boneSO->addComponent<CRenderable>();
				boneRenderable->setMesh(assets.sphereMesh);

				boneSO->setParent(fingerSO);
			}
		}

		handSO->setParent(handsSO);

		//handSO->setActive(false);
	}

	void setUpRigidHand(HSceneObject handsSO, eLeapHandType chirality)
	{
		String suffix = (chirality == eLeapHandType_Left) ? "_L" : "_R";

		HSceneObject handSO = SceneObject::create("RigidHand" + suffix);

		HLeapRigidHand handModel = handSO->addComponent<CLeapRigidHand>();
		handModel->mChirality = chirality;

		HLeapHandEnableDisable handTransition = handSO->addComponent<CLeapHandEnableDisable>();

		HSceneObject palmSO = SceneObject::create("palm");

		palmSO->setParent(handSO);

		for (UINT32 f = 0; f < 5; ++f)
		{
			HSceneObject fingerSO = SceneObject::create("finger" + toString(f));

			HLeapRigidFinger fingerModel = handSO->addComponent<CLeapRigidFinger>();

			handModel->mFingers[f] = fingerModel;

			fingerSO->setParent(handSO);

			for (UINT32 b = 0; b < 3; ++b)
			{
				HSceneObject boneSO = SceneObject::create("bone" + toString(b));

				fingerModel->mBones[b] = boneSO;

				HSphereCollider collider = boneSO->addComponent<CSphereCollider>();
				collider->setRadius(HAND_SPHERE_RADIUS);
				//collider->setHalfHeight(0.15f);
				//collider->setCenter(Vector3(0.15f, 0.0f, 0.0f));

				HRigidbody rigidbody = boneSO->addComponent<CRigidbody>();
				rigidbody->setIsKinematic(true);
				rigidbody->setMass(1e9f);

				boneSO->setParent(fingerSO);
			}
		}

		handSO->setParent(handsSO);

		//handSO->setActive(false);
	}

	/** Set up the scene used by the example, and the camera to view the world through. */
	void setUpScene()
	{
		/************************************************************************/
		/* 									ASSETS	                    		*/
		/************************************************************************/

		// Prepare all the resources we'll be using throughout this example

		// Grab a couple of test textures that we'll apply to the rendered objects
		HTexture gridPattern = ExampleFramework::loadTexture(ExampleTexture::GridPattern);
		HTexture gridPattern2 = ExampleFramework::loadTexture(ExampleTexture::GridPattern2);

		// Grab the default PBR shader
		HShader shader = gBuiltinResources().getBuiltinShader(BuiltinShader::Standard);
		
		// Create a set of materials to apply to renderables used
		HMaterial planeMaterial = Material::create(shader);
		planeMaterial->setTexture("gAlbedoTex", gridPattern2);

		// Tile the texture so every tile covers a 2x2m area
		planeMaterial->setVec2("gUVTile", Vector2::ONE * GROUND_PLANE_SCALE * 0.5f);

		HMaterial boxMaterial = Material::create(shader);
		boxMaterial->setTexture("gAlbedoTex", gridPattern);

		HMaterial sphereMaterial = Material::create(shader);

		// Load meshes we'll used for our rendered objects
		HMesh boxMesh = gBuiltinResources().getMesh(BuiltinMesh::Box);
		HMesh planeMesh = gBuiltinResources().getMesh(BuiltinMesh::Quad);
		HMesh sphereMesh = gBuiltinResources().getMesh(BuiltinMesh::Sphere);

		// Create a physics material we'll use for the box geometry, as well as the floor. The material has high
		// static and dynamic friction, with low restitution (low bounciness). Simulates a harder, rough, solid surface.
		HPhysicsMaterial boxPhysicsMaterial = PhysicsMaterial::create(1.0f, 1.0f, 0.0f);

		// Create a physics material for the sphere geometry, with higher bounciness. Simulates elasticity.
		HPhysicsMaterial spherePhysicsMaterial = PhysicsMaterial::create(1.0f, 1.0f, 0.5f);

		/************************************************************************/
		/* 									FLOOR	                    		*/
		/************************************************************************/

		// Set up renderable geometry for the floor plane
		HSceneObject floorSO = SceneObject::create("Floor");
		HRenderable floorRenderable = floorSO->addComponent<CRenderable>();
		floorRenderable->setMesh(planeMesh);
		floorRenderable->setMaterial(planeMaterial);

		floorSO->setScale(Vector3(GROUND_PLANE_SCALE, 1.0f, GROUND_PLANE_SCALE));

		// Add a plane collider that will prevent physical objects going through the floor
		HPlaneCollider planeCollider = floorSO->addComponent<CPlaneCollider>();

		// Apply the non-bouncy material
		planeCollider->setMaterial(boxPhysicsMaterial);

		/************************************************************************/
		/* 									BOXES	                    		*/
		/************************************************************************/

		// Helper method that creates a pyramid of six boxes that can be physically manipulated
		auto createBoxStack = [=](const Vector3& position, const Quaternion& rotation = Quaternion::IDENTITY)
		{
			HSceneObject boxSO[6];
			for (auto& entry : boxSO)
			{
				// Create a scene object and a renderable
				entry = SceneObject::create("Box");

				HRenderable boxRenderable = entry->addComponent<CRenderable>();
				boxRenderable->setMesh(boxMesh);
				boxRenderable->setMaterial(boxMaterial);

				// Add a plane collider that represent's the physical geometry of the box
				HBoxCollider boxCollider = entry->addComponent<CBoxCollider>();

				// Apply the non-bouncy material
				boxCollider->setMaterial(boxPhysicsMaterial);

				// Set the mass of a box to 25 kilograms
				boxCollider->setMass(25.0f);

				// Add a rigidbody, making the box geometry able to react to interactions with other physical objects
				HRigidbody boxRigidbody = entry->addComponent<CRigidbody>();
			}

			// Stack the boxes in a pyramid
			Vector3 positions[] =
			{
				// First row
				Vector3(-1.25f, 0.55f, 0.0f),
				Vector3(0.0f, 0.55f, 0.0f),
				Vector3(1.25f, 0.55f, 0.0f),
				// Second row
				Vector3(-0.65f, 1.6f, 0.0f),
				Vector3(0.65f, 1.6f, 0.0f),
				// Third row
				Vector3(0.0f, 2.65f, 0.0f),
			};

			for(UINT32 i = 0; i < 6; i++)
			{
				Vector3 pos = rotation.rotate(positions[i]) + position;
				boxSO[i]->setPosition(pos);
			}
		};

		createBoxStack(Vector3::ZERO);
		createBoxStack(Vector3(6.0f, 0.0f, 3.0f), Quaternion(Degree(0.0f), Degree(-45.0f), Degree(0.0f)));
		createBoxStack(Vector3(-6.0f, 0.0f, 3.0f), Quaternion(Degree(0.0f), Degree(45.0f), Degree(0.0f)));

		/************************************************************************/
		/* 									CHARACTER                    		*/
		/************************************************************************/

		// Add physics geometry and components for character movement and physics interaction
		HSceneObject characterSO = SceneObject::create("Character");
		characterSO->setPosition(Vector3(0.0f, 1.0f, 5.0f));

		// Add a character controller, representing the physical geometry of the character
		HCharacterController charController = characterSO->addComponent<CCharacterController>();

		// Make the character about 1.8m high, with 0.4m radius (controller represents a capsule)
		charController->setHeight(1.0f); // + 0.4 * 2 radius = 1.8m height
		charController->setRadius(0.4f);

		// FPS walker uses default input controls to move the character controller attached to the same object
		characterSO->addComponent<FPSWalker>();

		/************************************************************************/
		/* 									CAMERA	                     		*/
		/************************************************************************/

		// In order something to render on screen we need at least one camera.

		// Like before, we create a new scene object at (0, 0, 0).
		HSceneObject sceneCameraSO = SceneObject::create("SceneCamera");

		// Get the primary render window we need for creating the camera. 
		SPtr<RenderWindow> window = gApplication().getPrimaryWindow();

		// Add a Camera component that will output whatever it sees into that window 
		// (You could also use a render texture or another window you created).
		HCamera sceneCamera = sceneCameraSO->addComponent<CCamera>();
		sceneCamera->getViewport()->setTarget(window);

		// Set up camera component properties

		// Set closest distance that is visible. Anything below that is clipped.
		sceneCamera->setNearClipDistance(0.005f);

		// Set farthest distance that is visible. Anything above that is clipped.
		sceneCamera->setFarClipDistance(1000);

		// Set aspect ratio depending on the current resolution
		sceneCamera->setAspectRatio(windowResWidth / (float)windowResHeight);

		// Enable multi-sample anti-aliasing for better quality
		sceneCamera->setMSAACount(4);

		// Add a component that allows the camera to be rotated using the mouse
		HFPSCamera fpsCamera = sceneCameraSO->addComponent<FPSCamera>();

		// Set the character controller on the FPS camera, so the component can apply yaw rotation to it
		fpsCamera->setCharacter(characterSO);

		// Make the camera a child of the character scene object, and position it roughly at eye level
		sceneCameraSO->setParent(characterSO);
		sceneCameraSO->setPosition(Vector3(0.0f, 1.8f, 0.0f));

		/************************************************************************/
		/* 									SKYBOX                       		*/
		/************************************************************************/

		// Load a skybox texture
		HTexture skyCubemap = ExampleFramework::loadTexture(ExampleTexture::EnvironmentDaytime, false, true, true);

		// Add a skybox texture for sky reflections
		HSceneObject skyboxSO = SceneObject::create("Skybox");

		HSkybox skybox = skyboxSO->addComponent<CSkybox>();
		skybox->setTexture(skyCubemap);

		/************************************************************************/
		/* 									CURSOR                       		*/
		/************************************************************************/

		// Hide and clip the cursor, since we only use the mouse movement for camera rotation
		Cursor::instance().hide();
		Cursor::instance().clipToWindow(*window);

		/************************************************************************/
		/* 									INPUT                       		*/
		/************************************************************************/

		// Hook up input that launches a sphere when user clicks the mouse, and Esc key to quit
		gInput().onButtonUp.connect([=](const ButtonEvent& ev)
		{
			if(ev.buttonCode == BC_MOUSE_LEFT)
			{
				// Create the scene object and renderable geometry of the sphere
				HSceneObject sphereSO = SceneObject::create("Sphere");

				HRenderable sphereRenderable = sphereSO->addComponent<CRenderable>();
				sphereRenderable->setMesh(sphereMesh);
				sphereRenderable->setMaterial(sphereMaterial);

				// Create a spherical collider, represting physical geometry
				HSphereCollider sphereCollider = sphereSO->addComponent<CSphereCollider>();

				// Apply the bouncy material
				sphereCollider->setMaterial(spherePhysicsMaterial);

				// Set mass to 25kg
				sphereCollider->setMass(25.0f);

				// Add a rigidbody, making the object interactable
				HRigidbody sphereRigidbody = sphereSO->addComponent<CRigidbody>();
				
				// Position the sphere in front of the character, and scale it down a bit
				Vector3 spawnPos = characterSO->getTransform().getPosition();
				spawnPos += sceneCameraSO->getTransform().getForward() * 0.5f;
				spawnPos.y += 0.5f;

				sphereSO->setPosition(spawnPos);
				//sphereSO->setScale(Vector3(0.3f, 0.3f, 0.3f));

				// Apply force to the sphere, launching it forward in the camera's view direction
				sphereRigidbody->addForce(sceneCameraSO->getTransform().getForward() * 40.0f, ForceMode::Velocity);
			}
			else if(ev.buttonCode == BC_ESCAPE)
			{
				// Quit the application when Escape key is pressed
				gApplication().quitRequested();
			}
		});

		/************************************************************************/
		/* 									GUI		                     		*/
		/************************************************************************/

		// Display GUI elements indicating to the user which input keys are available

		// Add a GUIWidget component we will use for rendering the GUI
		HSceneObject guiSO = SceneObject::create("GUI");
		HGUIWidget gui = guiSO->addComponent<CGUIWidget>(sceneCamera);

		// Grab the main panel onto which to attach the GUI elements to
		GUIPanel* mainPanel = gui->getPanel();

		// Create a vertical GUI layout to align the labels one below each other
		GUILayoutY* vertLayout = GUILayoutY::create();

		// Create the GUI labels displaying the available input commands
		HString shootString(u8"Press left mouse button to shoot");
		HString quitString(u8"Press the Escape key to quit");

		vertLayout->addNewElement<GUILabel>(shootString);
		vertLayout->addNewElement<GUILabel>(quitString);

		// Register the layout with the main GUI panel, placing the layout in top left corner of the screen by default
		mainPanel->addElement(vertLayout);

		/************************************************************************/
		/* 									HANDS	                     		*/
		/************************************************************************/

		loadHandAssets();

		HSceneObject leapSO = SceneObject::create("LeapServiceProvider");
		leapSO->setPosition(leapProviderPos);
		leapSO->setScale(leapProviderScl);

		leapSO->addComponent<CLeapServiceProvider>();

		HSceneObject handsSO = SceneObject::create("Hands");

		HLeapHandModelManager handModels = handsSO->addComponent<CLeapHandModelManager>();

		setUpCapsuleHand(handsSO, eLeapHandType_Left);
		setUpCapsuleHand(handsSO, eLeapHandType_Right);

		HSceneObject capsuleL = handsSO->findChild("CapsuleHand_L");
		HSceneObject capsuleR = handsSO->findChild("CapsuleHand_R");

		handModels->addNewGroup("Capsule",
			capsuleL->getComponent<CLeapCapsuleHand>(),
			capsuleR->getComponent<CLeapCapsuleHand>());

		setUpRigidHand(handsSO, eLeapHandType_Left);
		setUpRigidHand(handsSO, eLeapHandType_Right);

		HSceneObject rigidL = handsSO->findChild("RigidHand_L");
		HSceneObject rigidR = handsSO->findChild("RigidHand_R");

		handModels->addNewGroup("Rigid",
			rigidL->getComponent<CLeapRigidHand>(),
			rigidR->getComponent<CLeapRigidHand>());
	}
}

/** Main entry point into the application. */
#if BS_PLATFORM == BS_PLATFORM_WIN32
#include <windows.h>

int CALLBACK WinMain(
	_In_  HINSTANCE hInstance,
	_In_  HINSTANCE hPrevInstance,
	_In_  LPSTR lpCmdLine,
	_In_  int nCmdShow
	)
#else
int main()
#endif
{
	using namespace bs;

	// Initializes the application and creates a window with the specified properties
	VideoMode videoMode(windowResWidth, windowResHeight);
	Application::startUp(videoMode, "Example", false);

	LeapService::startUp();

	// Registers a default set of input controls
	ExampleFramework::setupInputConfig();

	// Set up the scene with an object to render and a camera
	setUpScene();

	// Runs the main loop that does most of the work. This method will exit when user closes the main
	// window or exits in some other way.
	Application::instance().runMainLoop();

	// When done, clean up
	Application::shutDown();

	return 0;
}
