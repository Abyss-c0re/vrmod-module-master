#pragma once
#include <openvr/openvr.h>
#include <cstring>

namespace vr {

class MockIVRSystem : public IVRSystem {
public:
    uint32_t mockWidth = 1512, mockHeight = 1680;
    uint32_t mockDeviceCount = 0;
    char mockDeviceNames[16][256] = {};
    HmdMatrix44_t mockProjection = {};
    HmdMatrix34_t mockTransform = {};

    void GetRecommendedRenderTargetSize( uint32_t *pnWidth, uint32_t *pnHeight ) override { if(pnWidth)*pnWidth=mockWidth;if(pnHeight)*pnHeight=mockHeight; }
    HmdMatrix44_t GetProjectionMatrix( EVREye eEye, float fNearZ, float fFarZ ) override { return mockProjection; }
    void GetProjectionRaw( EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom ) override {  }
    bool ComputeDistortion( EVREye eEye, float fU, float fV, DistortionCoordinates_t *pDistortionCoordinates ) override { return false; }
    HmdMatrix34_t GetEyeToHeadTransform( EVREye eEye ) override { return mockTransform; }
    bool GetTimeSinceLastVsync( float *pfSecondsSinceLastVsync, uint64_t *pulFrameCounter ) override { return false; }
    int32_t GetD3D9AdapterIndex() override { return 0; }
    void GetDXGIOutputInfo( int32_t *pnAdapterIndex ) override {  }
    void GetOutputDevice( uint64_t *pnDevice, ETextureType textureType, VkInstance_T *pInstance = nullptr ) override {  }
    bool IsDisplayOnDesktop() override { return false; }
    bool SetDisplayVisibility( bool bIsVisibleOnDesktop ) override { return false; }
    void GetDeviceToAbsoluteTrackingPose( ETrackingUniverseOrigin eOrigin, float fPredictedSecondsToPhotonsFromNow, TrackedDevicePose_t *pTrackedDevicePoseArray, uint32_t unTrackedDevicePoseArrayCount ) override {  }
    HmdMatrix34_t GetSeatedZeroPoseToStandingAbsoluteTrackingPose() override { return {}; }
    HmdMatrix34_t GetRawZeroPoseToStandingAbsoluteTrackingPose() override { return {}; }
    uint32_t GetSortedTrackedDeviceIndicesOfClass( ETrackedDeviceClass eTrackedDeviceClass, vr::TrackedDeviceIndex_t *punTrackedDeviceIndexArray, uint32_t unTrackedDeviceIndexArrayCount, vr::TrackedDeviceIndex_t unRelativeToTrackedDeviceIndex = k_unTrackedDeviceIndex_Hmd ) override { return 0; }
    EDeviceActivityLevel GetTrackedDeviceActivityLevel( vr::TrackedDeviceIndex_t unDeviceId ) override { return {}; }
    void ApplyTransform( TrackedDevicePose_t *pOutputPose, const TrackedDevicePose_t *pTrackedDevicePose, const HmdMatrix34_t *pTransform ) override {  }
    vr::TrackedDeviceIndex_t GetTrackedDeviceIndexForControllerRole( vr::ETrackedControllerRole unDeviceType ) override { return {}; }
    vr::ETrackedControllerRole GetControllerRoleForTrackedDeviceIndex( vr::TrackedDeviceIndex_t unDeviceIndex ) override { return {}; }
    ETrackedDeviceClass GetTrackedDeviceClass( vr::TrackedDeviceIndex_t unDeviceIndex ) override { return {}; }
    bool IsTrackedDeviceConnected( vr::TrackedDeviceIndex_t unDeviceIndex ) override { return false; }
    bool GetBoolTrackedDeviceProperty( vr::TrackedDeviceIndex_t unDeviceIndex, ETrackedDeviceProperty prop, ETrackedPropertyError *pError = 0L ) override { return false; }
    float GetFloatTrackedDeviceProperty( vr::TrackedDeviceIndex_t unDeviceIndex, ETrackedDeviceProperty prop, ETrackedPropertyError *pError = 0L ) override { return 0; }
    int32_t GetInt32TrackedDeviceProperty( vr::TrackedDeviceIndex_t unDeviceIndex, ETrackedDeviceProperty prop, ETrackedPropertyError *pError = 0L ) override { return 0; }
    uint64_t GetUint64TrackedDeviceProperty( vr::TrackedDeviceIndex_t unDeviceIndex, ETrackedDeviceProperty prop, ETrackedPropertyError *pError = 0L ) override { return 0; }
    HmdMatrix34_t GetMatrix34TrackedDeviceProperty( vr::TrackedDeviceIndex_t unDeviceIndex, ETrackedDeviceProperty prop, ETrackedPropertyError *pError = 0L ) override { return {}; }
    uint32_t GetArrayTrackedDeviceProperty( vr::TrackedDeviceIndex_t unDeviceIndex, ETrackedDeviceProperty prop, PropertyTypeTag_t propType, void *pBuffer, uint32_t unBufferSize, ETrackedPropertyError *pError = 0L ) override { return 0; }
    uint32_t GetStringTrackedDeviceProperty( vr::TrackedDeviceIndex_t unDeviceIndex, ETrackedDeviceProperty prop, char *pchValue, uint32_t unBufferSize, ETrackedPropertyError *pError = 0L ) override { if(unDeviceIndex<mockDeviceCount&&prop==Prop_ControllerType_String){strncpy(pchValue,mockDeviceNames[unDeviceIndex],unBufferSize-1);pchValue[unBufferSize-1]=0;return(uint32_t)strlen(mockDeviceNames[unDeviceIndex])+1;}return 0; }
    const char *GetPropErrorNameFromEnum( ETrackedPropertyError error ) override { return ""; }
    bool PollNextEvent( VREvent_t *pEvent, uint32_t uncbVREvent ) override { return false; }
    bool PollNextEventWithPose( ETrackingUniverseOrigin eOrigin, VREvent_t *pEvent, uint32_t uncbVREvent, vr::TrackedDevicePose_t *pTrackedDevicePose ) override { return false; }
    const char *GetEventTypeNameFromEnum( EVREventType eType ) override { return ""; }
    HiddenAreaMesh_t GetHiddenAreaMesh( EVREye eEye, EHiddenAreaMeshType type = k_eHiddenAreaMesh_Standard ) override { return {}; }
    bool GetControllerState( vr::TrackedDeviceIndex_t unControllerDeviceIndex, vr::VRControllerState_t *pControllerState, uint32_t unControllerStateSize ) override { return false; }
    bool GetControllerStateWithPose( ETrackingUniverseOrigin eOrigin, vr::TrackedDeviceIndex_t unControllerDeviceIndex, vr::VRControllerState_t *pControllerState, uint32_t unControllerStateSize, TrackedDevicePose_t *pTrackedDevicePose ) override { return false; }
    void TriggerHapticPulse( vr::TrackedDeviceIndex_t unControllerDeviceIndex, uint32_t unAxisId, unsigned short usDurationMicroSec ) override {  }
    const char *GetButtonIdNameFromEnum( EVRButtonId eButtonId ) override { return ""; }
    const char *GetControllerAxisTypeNameFromEnum( EVRControllerAxisType eAxisType ) override { return ""; }
    bool IsInputAvailable() override { return false; }
    bool IsSteamVRDrawingControllers() override { return false; }
    bool ShouldApplicationPause() override { return false; }
    bool ShouldApplicationReduceRenderingWork() override { return false; }
    vr::EVRFirmwareError PerformFirmwareUpdate( vr::TrackedDeviceIndex_t unDeviceIndex ) override { return {}; }
    void AcknowledgeQuit_Exiting() override {  }
    uint32_t GetAppContainerFilePaths( char *pchBuffer, uint32_t unBufferSize ) override { return 0; }
    const char *GetRuntimeVersion() override { return ""; }
};

class MockIVRCompositor : public IVRCompositor {
public:
    int submitCount = 0;

    void SetTrackingSpace( ETrackingUniverseOrigin eOrigin ) override {  }
    ETrackingUniverseOrigin GetTrackingSpace() override { return {}; }
    EVRCompositorError WaitGetPoses( TrackedDevicePose_t* pRenderPoseArray, uint32_t unRenderPoseArrayCount, TrackedDevicePose_t* pGamePoseArray, uint32_t unGamePoseArrayCount ) override { return VRCompositorError_None; }
    EVRCompositorError GetLastPoses( TrackedDevicePose_t* pRenderPoseArray, uint32_t unRenderPoseArrayCount, TrackedDevicePose_t* pGamePoseArray, uint32_t unGamePoseArrayCount ) override { return {}; }
    EVRCompositorError GetLastPoseForTrackedDeviceIndex( TrackedDeviceIndex_t unDeviceIndex, TrackedDevicePose_t *pOutputPose, TrackedDevicePose_t *pOutputGamePose ) override { return {}; }
    EVRCompositorError Submit( EVREye eEye, const Texture_t *pTexture, const VRTextureBounds_t* pBounds = 0, EVRSubmitFlags nSubmitFlags = Submit_Default ) override { submitCount++;return VRCompositorError_None; }
    EVRCompositorError SubmitWithArrayIndex( EVREye eEye, const Texture_t *pTexture, uint32_t unTextureArrayIndex, const VRTextureBounds_t *pBounds = 0, EVRSubmitFlags nSubmitFlags = Submit_Default ) override { return {}; }
    void ClearLastSubmittedFrame() override {  }
    void PostPresentHandoff() override {  }
    bool GetFrameTiming( Compositor_FrameTiming *pTiming, uint32_t unFramesAgo = 0 ) override { return false; }
    uint32_t GetFrameTimings( Compositor_FrameTiming *pTiming, uint32_t nFrames ) override { return 0; }
    float GetFrameTimeRemaining() override { return 0; }
    void GetCumulativeStats( Compositor_CumulativeStats *pStats, uint32_t nStatsSizeInBytes ) override {  }
    void FadeToColor( float fSeconds, float fRed, float fGreen, float fBlue, float fAlpha, bool bBackground = false ) override {  }
    HmdColor_t GetCurrentFadeColor( bool bBackground = false ) override { return {}; }
    void FadeGrid( float fSeconds, bool bFadeGridIn ) override {  }
    float GetCurrentGridAlpha() override { return 0; }
    EVRCompositorError SetSkyboxOverride( const Texture_t *pTextures, uint32_t unTextureCount ) override { return {}; }
    void ClearSkyboxOverride() override {  }
    void CompositorBringToFront() override {  }
    void CompositorGoToBack() override {  }
    void CompositorQuit() override {  }
    bool IsFullscreen() override { return false; }
    uint32_t GetCurrentSceneFocusProcess() override { return 0; }
    uint32_t GetLastFrameRenderer() override { return 0; }
    bool CanRenderScene() override { return false; }
    void ShowMirrorWindow() override {  }
    void HideMirrorWindow() override {  }
    bool IsMirrorWindowVisible() override { return false; }
    void CompositorDumpImages() override {  }
    bool ShouldAppRenderWithLowResources() override { return false; }
    void ForceInterleavedReprojectionOn( bool bOverride ) override {  }
    void ForceReconnectProcess() override {  }
    void SuspendRendering( bool bSuspend ) override {  }
    vr::EVRCompositorError GetMirrorTextureD3D11( vr::EVREye eEye, void *pD3D11DeviceOrResource, void **ppD3D11ShaderResourceView ) override { return {}; }
    void ReleaseMirrorTextureD3D11( void *pD3D11ShaderResourceView ) override {  }
    vr::EVRCompositorError GetMirrorTextureGL( vr::EVREye eEye, vr::glUInt_t *pglTextureId, vr::glSharedTextureHandle_t *pglSharedTextureHandle ) override { return {}; }
    bool ReleaseSharedGLTexture( vr::glUInt_t glTextureId, vr::glSharedTextureHandle_t glSharedTextureHandle ) override { return false; }
    void LockGLSharedTextureForAccess( vr::glSharedTextureHandle_t glSharedTextureHandle ) override {  }
    void UnlockGLSharedTextureForAccess( vr::glSharedTextureHandle_t glSharedTextureHandle ) override {  }
    uint32_t GetVulkanInstanceExtensionsRequired( char *pchValue, uint32_t unBufferSize ) override { return 0; }
    uint32_t GetVulkanDeviceExtensionsRequired( VkPhysicalDevice_T *pPhysicalDevice, char *pchValue, uint32_t unBufferSize ) override { return 0; }
    void SetExplicitTimingMode( EVRCompositorTimingMode eTimingMode ) override {  }
    EVRCompositorError SubmitExplicitTimingData() override { return {}; }
    bool IsMotionSmoothingEnabled() override { return false; }
    bool IsMotionSmoothingSupported() override { return false; }
    bool IsCurrentSceneFocusAppLoading() override { return false; }
    EVRCompositorError SetStageOverride_Async( const char *pchRenderModelPath, const HmdMatrix34_t *pTransform = 0, const Compositor_StageRenderSettings *pRenderSettings = 0, uint32_t nSizeOfRenderSettings = 0 ) override { return {}; }
    void ClearStageOverride() override {  }
    bool GetCompositorBenchmarkResults( Compositor_BenchmarkResults *pBenchmarkResults, uint32_t nSizeOfBenchmarkResults ) override { return false; }
    EVRCompositorError GetLastPosePredictionIDs( uint32_t *pRenderPosePredictionID, uint32_t *pGamePosePredictionID ) override { return {}; }
    EVRCompositorError GetPosesForFrame( uint32_t unPosePredictionID, TrackedDevicePose_t* pPoseArray, uint32_t unPoseArrayCount ) override { return {}; }
};

class MockIVRInput : public IVRInput {
public:
    uint64_t mockHandleCounter = 100, mockSetHandleCounter = 200;
    bool hapticTriggered = false;
    VRActionHandle_t lastHapticAction = 0;
    float lastHapticAmplitude = 0;
    InputDigitalActionData_t mockDigitalData[64] = {};
    int mockDigitalCount = 0, mockDigitalIndex = 0;
    InputAnalogActionData_t mockAnalogData[64] = {};
    int mockAnalogCount = 0, mockAnalogIndex = 0;

    EVRInputError SetActionManifestPath( const char *pchActionManifestPath ) override { return VRInputError_None; }
    EVRInputError GetActionSetHandle( const char *pchActionSetName, VRActionSetHandle_t *pHandle ) override { *pHandle=++mockSetHandleCounter;return VRInputError_None; }
    EVRInputError GetActionHandle( const char *pchActionName, VRActionHandle_t *pHandle ) override { *pHandle=++mockHandleCounter;return VRInputError_None; }
    EVRInputError GetInputSourceHandle( const char *pchInputSourcePath, VRInputValueHandle_t *pHandle ) override { return {}; }
    EVRInputError UpdateActionState( VRActiveActionSet_t *pSets, uint32_t unSizeOfVRSelectedActionSet_t, uint32_t unSetCount ) override { return VRInputError_None; }
    EVRInputError GetDigitalActionData( VRActionHandle_t action, InputDigitalActionData_t *pActionData, uint32_t unActionDataSize, VRInputValueHandle_t ulRestrictToDevice ) override { if(mockDigitalIndex<mockDigitalCount)*pActionData=mockDigitalData[mockDigitalIndex++];return VRInputError_None; }
    EVRInputError GetAnalogActionData( VRActionHandle_t action, InputAnalogActionData_t *pActionData, uint32_t unActionDataSize, VRInputValueHandle_t ulRestrictToDevice ) override { if(mockAnalogIndex<mockAnalogCount)*pActionData=mockAnalogData[mockAnalogIndex++];return VRInputError_None; }
    EVRInputError GetPoseActionDataRelativeToNow( VRActionHandle_t action, ETrackingUniverseOrigin eOrigin, float fPredictedSecondsFromNow, InputPoseActionData_t *pActionData, uint32_t unActionDataSize, VRInputValueHandle_t ulRestrictToDevice ) override { return VRInputError_None; }
    EVRInputError GetPoseActionDataForNextFrame( VRActionHandle_t action, ETrackingUniverseOrigin eOrigin, InputPoseActionData_t *pActionData, uint32_t unActionDataSize, VRInputValueHandle_t ulRestrictToDevice ) override { return VRInputError_None; }
    EVRInputError GetSkeletalActionData( VRActionHandle_t action, InputSkeletalActionData_t *pActionData, uint32_t unActionDataSize ) override { return {}; }
    EVRInputError GetDominantHand( ETrackedControllerRole *peDominantHand ) override { return {}; }
    EVRInputError SetDominantHand( ETrackedControllerRole eDominantHand ) override { return {}; }
    EVRInputError GetBoneCount( VRActionHandle_t action, uint32_t* pBoneCount ) override { return {}; }
    EVRInputError GetBoneHierarchy( VRActionHandle_t action, BoneIndex_t* pParentIndices, uint32_t unIndexArayCount ) override { return {}; }
    EVRInputError GetBoneName( VRActionHandle_t action, BoneIndex_t nBoneIndex, char* pchBoneName, uint32_t unNameBufferSize ) override { return {}; }
    EVRInputError GetSkeletalReferenceTransforms( VRActionHandle_t action, EVRSkeletalTransformSpace eTransformSpace, EVRSkeletalReferencePose eReferencePose, VRBoneTransform_t *pTransformArray, uint32_t unTransformArrayCount ) override { return {}; }
    EVRInputError GetSkeletalTrackingLevel( VRActionHandle_t action, EVRSkeletalTrackingLevel* pSkeletalTrackingLevel ) override { return {}; }
    EVRInputError GetSkeletalBoneData( VRActionHandle_t action, EVRSkeletalTransformSpace eTransformSpace, EVRSkeletalMotionRange eMotionRange, VRBoneTransform_t *pTransformArray, uint32_t unTransformArrayCount ) override { return {}; }
    EVRInputError GetSkeletalSummaryData( VRActionHandle_t action, EVRSummaryType eSummaryType, VRSkeletalSummaryData_t * pSkeletalSummaryData ) override { return VRInputError_None; }
    EVRInputError GetSkeletalBoneDataCompressed( VRActionHandle_t action, EVRSkeletalMotionRange eMotionRange, void *pvCompressedData, uint32_t unCompressedSize, uint32_t *punRequiredCompressedSize ) override { return {}; }
    EVRInputError DecompressSkeletalBoneData( const void *pvCompressedBuffer, uint32_t unCompressedBufferSize, EVRSkeletalTransformSpace eTransformSpace, VRBoneTransform_t *pTransformArray, uint32_t unTransformArrayCount ) override { return {}; }
    EVRInputError TriggerHapticVibrationAction( VRActionHandle_t action, float fStartSecondsFromNow, float fDurationSeconds, float fFrequency, float fAmplitude, VRInputValueHandle_t ulRestrictToDevice ) override { hapticTriggered=true;lastHapticAction=action;lastHapticAmplitude=fAmplitude;return VRInputError_None; }
    EVRInputError GetActionOrigins( VRActionSetHandle_t actionSetHandle, VRActionHandle_t digitalActionHandle, VRInputValueHandle_t *originsOut, uint32_t originOutCount ) override { return {}; }
    EVRInputError GetOriginLocalizedName( VRInputValueHandle_t origin, char *pchNameArray, uint32_t unNameArraySize, int32_t unStringSectionsToInclude ) override { return {}; }
    EVRInputError GetOriginTrackedDeviceInfo( VRInputValueHandle_t origin, InputOriginInfo_t *pOriginInfo, uint32_t unOriginInfoSize ) override { return {}; }
    EVRInputError GetActionBindingInfo( VRActionHandle_t action, InputBindingInfo_t *pOriginInfo, uint32_t unBindingInfoSize, uint32_t unBindingInfoCount, uint32_t *punReturnedBindingInfoCount ) override { return {}; }
    EVRInputError ShowActionOrigins( VRActionSetHandle_t actionSetHandle, VRActionHandle_t ulActionHandle ) override { return {}; }
    EVRInputError ShowBindingsForActionSet( VRActiveActionSet_t *pSets, uint32_t unSizeOfVRSelectedActionSet_t, uint32_t unSetCount, VRInputValueHandle_t originToHighlight ) override { return {}; }
    EVRInputError GetComponentStateForBinding( const char *pchRenderModelName, const char *pchComponentName, const InputBindingInfo_t *pOriginInfo, uint32_t unBindingInfoSize, uint32_t unBindingInfoCount, vr::RenderModel_ComponentState_t *pComponentState ) override { return {}; }
    bool IsUsingLegacyInput() override { return false; }
    EVRInputError OpenBindingUI( const char* pchAppKey, VRActionSetHandle_t ulActionSetHandle, VRInputValueHandle_t ulDeviceHandle, bool bShowOnDesktop ) override { return {}; }
    EVRInputError GetBindingVariant( vr::VRInputValueHandle_t ulDevicePath, char *pchVariantArray, uint32_t unVariantArraySize ) override { return {}; }
};

} // namespace vr
