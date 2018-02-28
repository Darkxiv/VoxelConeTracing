#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <GlobalUtils.h>

enum RenderOutput
{
    RO_COLOR,
    RO_INDIRECT,
    RO_BRICKS,
    RO_VOXELS,

    RO_COUNT
};

enum DebugBuffer
{
    DBUF_OPACITY,
    DBUF_IRRADIANCE,

    DBUF_COUNT
};

class Settings
{
public:
    static Settings& Get( )
    {
        static Settings mSettings;
        return mSettings;
    }
    Settings( const Settings& ) = delete; // Prevent copy-construction
    Settings& operator=( const Settings& ) = delete; // Prevent assignment

    // D3DRenderer settings
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int mWndWidth;
    int mWndHeight;

    RenderOutput mRenderOutput;
    int mShadowMapRes;
    float mShadowMapZNear;
    float mShadowMapZFar;
    float mShadowBias;

    float mBlurSharpness;
    float mBlurRadius;

    float mAOInfluence;
    float mDirectInfluence;
    float mIndirectInfluence;

    // VCT settings
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool mVCTEnable;
    int mOctreeHeight;
    int mOctreeBufferRes;
    int mBrickBufferRes;

    int mVCTDebugOctreeLevel;
    DebugBuffer mVCTDebugBuffer;

    bool mVCTDebugView;
    int mVCTDebugConeDir;

    int mVCTDebugOctreeFirstLevel;
    int mVCTDebugOctreeLastLevel;

    float mVCTLambdaFalloff;
    float mVCTLocalConeOffset;
    float mVCTWorldConeOffset;
    float mVCTIndirectAmplification;
    float mVCTStepCorrection;
    bool mVCTUseOpacityBuffer;
    int mVCTConeTracingRes;

    bool mShowAO;

    // Scene settings
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    float mSunYaw;
    float mSunPitch;
    float mLightDistance;
    float mSunColor[3];
    float mSunPower;

    bool mLightAnimation;
    float mLightAnimationSpeed;

    float mMouseSens;
    float mInitCamPos[3];
    float mInitCamPhi;
    float mInitCamTheta;
    float mCameraSpeed;

    // Common settings
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    float mFrameDeltaTime;

    char *mShaderDir;
    char *mMediaDir;

    char *mSceneFn;
    char *mSaveSceneFn;
    bool mSaveScene;

private:
    Settings();
};

#endif