#
# Copyright (c) 2015 NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := nouveau_simplest
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES += libdrm_nouveau
LOCAL_SHARED_LIBRARIES += libsync

LOCAL_C_INCLUDES += external/libdrm
LOCAL_C_INCLUDES += external/libdrm/include/drm
LOCAL_C_INCLUDES += system/core/libsync/include
LOCAL_C_INCLUDES += system/core/libsync

LOCAL_SRC_FILES += main.c

include $(BUILD_EXECUTABLE)
