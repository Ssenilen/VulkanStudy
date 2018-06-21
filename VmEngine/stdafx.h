// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �Ǵ� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>

#include <cassert>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
//#include <vulkan/vk_sdk_platform.h>
//#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
// glm
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtx\hash.hpp>
#include <glm\gtx\quaternion.hpp>
#include <glm\gtx\euler_angles.hpp>

#include "linmath.h"

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "libfbxsdk-md.lib")

// Memory Leak Check Define
#if defined(DEBUG) | defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h> // �̹� ���� ����
#include <crtdbg.h>
#endif
//
//#ifdef  _CRTDBG_MAP_ALLOC
//
//#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   calloc(c, s)      _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   _expand(p, s)     _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   free(p)           _free_dbg(p, _NORMAL_BLOCK)
//#define   _msize(p)         _msize_dbg(p, _NORMAL_BLOCK)
//
//#endif  /* _CRTDBG_MAP_ALLOC */