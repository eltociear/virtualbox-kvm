/* $Id: VBoxDX.h $ */
/** @file
 * VBoxVideo Display D3D User mode dll
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_wddm_dx_VBoxDX_h
#define GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_wddm_dx_VBoxDX_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>
#include <iprt/handletable.h>

#include <VBoxMPIf.h>

#pragma pack(1) /* VMSVGA structures are '__packed'. */
#include <svga3d_caps.h>
#include <svga3d_reg.h>
#include <svga3d_shaderdefs.h>
#include <svga_escape.h>
#include <svga_overlay.h>
#pragma pack()

#include <d3d11_1.h>

#ifdef DEBUG_sunlover
#define DEBUG_BREAKPOINT_TEST() do { ASMBreakpoint(); } while (0)
#else
#define DEBUG_BREAKPOINT_TEST() do { } while (0)
#endif

typedef struct VBOXDXADAPTER
{
    HANDLE hRTAdapter;
    UINT uIfVersion;
    UINT uRtVersion;
    D3DDDI_ADAPTERCALLBACKS RtCallbacks;

    VBOXVIDEO_HWTYPE enmHwType;     /* VBOXVIDEO_HWTYPE_* */

    bool f3D;

    VBOXWDDM_QAI AdapterInfo;
} VBOXDXADAPTER, *PVBOXDXADAPTER;

typedef struct VBOXDX_BLENDSTATE
{
    D3D10DDI_HRTBLENDSTATE hRTBlendState;
    D3D11_1_DDI_BLEND_DESC BlendDesc;
    uint32_t uBlendId;
} VBOXDX_BLENDSTATE, *PVBOXDX_BLENDSTATE;

typedef struct VBOXDX_DEPTHSTENCIL_STATE
{
    D3D10DDI_HRTDEPTHSTENCILSTATE hRTDepthStencilState;
    D3D10_DDI_DEPTH_STENCIL_DESC DepthStencilDesc;
    uint32_t uDepthStencilId;
} VBOXDX_DEPTHSTENCIL_STATE, *PVBOXDX_DEPTHSTENCIL_STATE;

typedef struct VBOXDX_RASTERIZER_STATE
{
    D3D10DDI_HRTRASTERIZERSTATE hRTRasterizerState;
    D3D11_1_DDI_RASTERIZER_DESC RasterizerDesc;
    uint32_t uRasterizerId;
} VBOXDX_RASTERIZER_STATE, *PVBOXDX_RASTERIZER_STATE;

typedef struct VBOXDX_SAMPLER_STATE
{
    D3D10DDI_HRTSAMPLER hRTSampler;
    D3D10_DDI_SAMPLER_DESC SamplerDesc;
    uint32_t uSamplerId;
} VBOXDX_SAMPLER_STATE, *PVBOXDX_SAMPLER_STATE;

typedef struct VBOXDXELEMENTLAYOUT
{
    D3D10DDI_HRTELEMENTLAYOUT      hRTElementLayout;
    UINT                           NumElements;
    uint32_t                       uElementLayoutId;
    RT_FLEXIBLE_ARRAY_EXTENSION
    D3D10DDIARG_INPUT_ELEMENT_DESC aVertexElements[RT_FLEXIBLE_ARRAY];
} VBOXDXELEMENTLAYOUT, *PVBOXDXELEMENTLAYOUT;

/*
 * A Context Object Allocation holds up to VBOXDX_COALLOCATION_MAX_OBJECTS objects (queries, etc)
 */
#define VBOXDX_COALLOCATION_MAX_OBJECTS (64)                /* Use a 64 bit field as free bitmap. */
typedef struct VBOXDXCOALLOCATION                           /* Allocation for context objects (query, etc). */
{
    RTLISTNODE                  nodeAllocationsChain;       /* Allocations can be chained. */
    D3DKMT_HANDLE               hCOAllocation;              /* Allocation handle. */
    uint32_t                    cbAllocation;               /* Size of this allocation. */
    uint64_t                    u64Bitmap;                  /* Bitmap of allocated blocks. */
    uint32_t                    aOffset[VBOXDX_COALLOCATION_MAX_OBJECTS]; /* Start offsets of blocks. */
} VBOXDXCOALLOCATION, *PVBOXDXCOALLOCATION;

typedef struct VBOXDXSHADER
{
    RTLISTNODE                  node;                       /* VBOXDX_DEVICE::listShaders */
    D3D10DDI_HRTSHADER          hRTShader;
    uint32_t                    uShaderId;
    SVGA3dShaderType            enmShaderType;
    uint32_t                    cbShader;                   /* Size of the shader code in bytes. */
    uint32_t                    cbSignatures;               /* Size of the shader signatures, which follow the shader bytecode. */
    uint32_t                    offShader;                  /* Offset of the shader in the shader allocation. */
    uint32_t                   *pu32Bytecode;               /* Bytecode follows the VBOXDXSHADER structure. */
    SVGA3dDXSignatureHeader    *pSignatures;                /* Signatures follow the bytecode. */
    struct
    {
        UINT                    NumEntries;
        UINT                    NumStrides;
        UINT                    BufferStridesInBytes[SVGA3D_DX_MAX_SOTARGETS];
        UINT                    RasterizedStream;
        uint32_t                uStreamOutputId;            /* Id of the stream output. */
        uint32_t                offStreamOutputDecls;       /* Offset of the declarations in the CO allocation. */
        uint32_t                cbOutputStreamDecls;
        PVBOXDXCOALLOCATION     pCOAllocation;
    } gs;                                                   /* For GS. */
    /* shader bytecode */
    /* shader signatures */
} VBOXDXSHADER, *PVBOXDXSHADER;

typedef enum VBOXDXQUERYSTATE
{
    VBOXDXQUERYSTATE_CREATED = 0,
    VBOXDXQUERYSTATE_BUILDING,
    VBOXDXQUERYSTATE_ISSUED,
    VBOXDXQUERYSTATE_SIGNALED
} VBOXDXQUERYSTATE;

/*
 * Queries are stored in allocations, each of which holds an array of queries of the same type.
 */
typedef struct VBOXDXQUERY
{
    RTLISTNODE                  nodeQuery;                  /* VBOXDX_DEVICE::listQueries */
    D3D10DDI_HRTQUERY           hRTQuery;
    D3D10DDI_QUERY              Query;
    struct
    {
        SVGA3dQueryType         queryType;
        SVGA3dDXQueryFlags      flags;
    } svga;
    VBOXDXQUERYSTATE            enmQueryState;
    uint32_t                    uQueryId;
    uint32_t                    offQuery;                   /* Offset of the query in the query allocation. */
    PVBOXDXCOALLOCATION         pCOAllocation;
    uint64_t u64Value;
    /* Result for queries in SIGNALED state. */
} VBOXDXQUERY, *PVBOXDXQUERY;

/*
 * This structure is allocated by the driver. It is used for accounting of resources
 * and for storing information which might be needed after the resource is destroyed by runtime.
 * KM stands for Kernel Mode.
 */
typedef struct VBOXDXKMRESOURCE
{
    RTLISTNODE                  nodeResource;               /* VBOXDX_DEVICE::listResources, listDestroyedResources. */
    struct VBOXDX_RESOURCE     *pResource;                  /* The structure allocated by D3D runtime. */
    D3DKMT_HANDLE               hAllocation;
    RTLISTNODE                  nodeStaging;                /* VBOXDX_DEVICE::listStagingResources if this resource is a staging buffer. */
} VBOXDXKMRESOURCE, *PVBOXDXKMRESOURCE;

typedef struct VBOXDX_RESOURCE
{
    PVBOXDXKMRESOURCE              pKMResource;
    //RTLISTNODE                     nodeResource;            /* VBOXDX_DEVICE::listResources. */
    D3D10DDI_HRTRESOURCE           hRTResource;
    D3D10DDIRESOURCE_TYPE          ResourceDimension;
    D3D10_DDI_RESOURCE_USAGE       Usage;
    VBOXDXALLOCATIONDESC           AllocationDesc;
    UINT                           cSubresources;
    union
    {
        D3D10_DDI_MAP              DDIMap;
        UINT                       uMap;
    };

    RTLISTANCHOR                   listSRV;                 /* Shader resource views created for this resource. */
    RTLISTANCHOR                   listRTV;                 /* Render target views. */
    RTLISTANCHOR                   listDSV;                 /* Depth stencil views. */
    RTLISTANCHOR                   listUAV;                 /* Unordered access views. */

    RT_FLEXIBLE_ARRAY_EXTENSION
    D3D10DDI_MIPINFO               aMipInfoList[RT_FLEXIBLE_ARRAY]; /** @todo Remove this array if it will not be needed. */
} VBOXDX_RESOURCE, *PVBOXDX_RESOURCE;

typedef struct VBOXDXSHADERRESOURCEVIEW
{
    RTLISTNODE                     nodeView;                /* listSRV of the resource. */
    bool                           fDefined : 1;            /* Whether the view has been defined on the host. */
    D3D10DDI_HRTSHADERRESOURCEVIEW hRTShaderResourceView;
    uint32_t                       uShaderResourceViewId;
    PVBOXDX_RESOURCE               pResource;
    DXGI_FORMAT                    Format;
    D3D10DDIRESOURCE_TYPE          ResourceDimension;
    union
    {
        D3D10DDIARG_BUFFER_SHADERRESOURCEVIEW    Buffer;
        D3D10DDIARG_TEX1D_SHADERRESOURCEVIEW     Tex1D;
        D3D10DDIARG_TEX2D_SHADERRESOURCEVIEW     Tex2D;
        D3D10DDIARG_TEX3D_SHADERRESOURCEVIEW     Tex3D;
        D3D10_1DDIARG_TEXCUBE_SHADERRESOURCEVIEW TexCube;
        D3D11DDIARG_BUFFEREX_SHADERRESOURCEVIEW  BufferEx;
    } DimensionDesc;
    struct
    {
        SVGA3dSurfaceFormat        format;
        SVGA3dResourceType         resourceDimension;
        SVGA3dShaderResourceViewDesc desc;
    } svga;
} VBOXDXSHADERRESOURCEVIEW, *PVBOXDXSHADERRESOURCEVIEW;

typedef struct VBOXDXRENDERTARGETVIEW
{
    RTLISTNODE                     nodeView;                /* listRTV of the resource. */
    bool                           fDefined : 1;            /* Whether the view has been defined on the host. */
    D3D10DDI_HRTRENDERTARGETVIEW   hRTRenderTargetView;
    uint32_t                       uRenderTargetViewId;
    PVBOXDX_RESOURCE               pResource;
    DXGI_FORMAT                    Format;
    D3D10DDIRESOURCE_TYPE          ResourceDimension;
    union
    {
        D3D10DDIARG_BUFFER_RENDERTARGETVIEW  Buffer;
        D3D10DDIARG_TEX1D_RENDERTARGETVIEW   Tex1D;
        D3D10DDIARG_TEX2D_RENDERTARGETVIEW   Tex2D;
        D3D10DDIARG_TEX3D_RENDERTARGETVIEW   Tex3D;
        D3D10DDIARG_TEXCUBE_RENDERTARGETVIEW TexCube;
    } DimensionDesc;
    struct
    {
        SVGA3dSurfaceFormat        format;
        SVGA3dResourceType         resourceDimension;
        SVGA3dRenderTargetViewDesc desc;
    } svga;
} VBOXDXRENDERTARGETVIEW, *PVBOXDXRENDERTARGETVIEW;

typedef struct VBOXDXDEPTHSTENCILVIEW
{
    RTLISTNODE                     nodeView;                /* listDSV of the resource. */
    bool                           fDefined : 1;            /* Whether the view has been defined on the host. */
    D3D10DDI_HRTDEPTHSTENCILVIEW   hRTDepthStencilView;
    uint32_t                       uDepthStencilViewId;
    PVBOXDX_RESOURCE               pResource;
    DXGI_FORMAT                    Format;
    D3D10DDIRESOURCE_TYPE          ResourceDimension;
    UINT                           Flags;
    union
    {
        D3D10DDIARG_TEX1D_DEPTHSTENCILVIEW   Tex1D;
        D3D10DDIARG_TEX2D_DEPTHSTENCILVIEW   Tex2D;
        D3D10DDIARG_TEXCUBE_DEPTHSTENCILVIEW TexCube;
    } DimensionDesc;
    struct
    {
        SVGA3dSurfaceFormat        format;
        SVGA3dResourceType         resourceDimension;
        uint32_t                   mipSlice;
        uint32_t                   firstArraySlice;
        uint32_t                   arraySize;
        SVGA3DCreateDSViewFlags    flags;
    } svga;
} VBOXDXDEPTHSTENCILVIEW, *PVBOXDXDEPTHSTENCILVIEW;


typedef struct VBOXDXUNORDEREDACCESSVIEW
{
    RTLISTNODE                     nodeView;                /* listUAV of the resource. */
    bool                           fDefined : 1;            /* Whether the view has been defined on the host. */
    D3D11DDI_HRTUNORDEREDACCESSVIEW hRTUnorderedAccessView;
    uint32_t                       uUnorderedAccessViewId;
    PVBOXDX_RESOURCE               pResource;
    DXGI_FORMAT                    Format;
    D3D10DDIRESOURCE_TYPE          ResourceDimension;
    union
    {
        D3D11DDIARG_BUFFER_UNORDEREDACCESSVIEW    Buffer;
        D3D11DDIARG_TEX1D_UNORDEREDACCESSVIEW     Tex1D;
        D3D11DDIARG_TEX2D_UNORDEREDACCESSVIEW     Tex2D;
        D3D11DDIARG_TEX3D_UNORDEREDACCESSVIEW     Tex3D;
    } DimensionDesc;
    struct
    {
        SVGA3dSurfaceFormat        format;
        SVGA3dResourceType         resourceDimension;
        SVGA3dUAViewDesc           desc;
    } svga;
} VBOXDXUNORDEREDACCESSVIEW, *PVBOXDXUNORDEREDACCESSVIEW;


typedef struct VBOXDXCONSTANTBUFFERSSTATE
{
    UINT                           StartSlot;
    UINT                           NumBuffers;
    VBOXDX_RESOURCE               *apResource[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    UINT                           aFirstConstant[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    UINT                           aNumConstants[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
} VBOXDXCONSTANTBUFFERSSTATE, *PVBOXDXCONSTANTBUFFERSSTATE;


typedef struct VBOXDXVERTEXBUFFERSSTATE
{
    UINT                           StartSlot;
    UINT                           NumBuffers;
    VBOXDX_RESOURCE               *apResource[SVGA3D_MAX_VERTEX_ARRAYS];
    UINT                           aStrides[SVGA3D_MAX_VERTEX_ARRAYS];
    UINT                           aOffsets[SVGA3D_MAX_VERTEX_ARRAYS];
} VBOXDXVERTEXBUFFERSSTATE, *PVBOXDXVERTEXBUFFERSSTATE;


typedef struct VBOXDXINDEXBUFFERSTATE
{
    PVBOXDX_RESOURCE               pBuffer;
    DXGI_FORMAT                    Format;
    UINT                           Offset;
} VBOXDXINDEXBUFFERSTATE, *PVBOXDXINDEXBUFFERSTATE;

typedef struct VBOXDX_DEVICE
{
    /* DX runtime data. */
    D3D10DDI_HRTDEVICE             hRTDevice;                /* Handle to use when calling 'pRTCallbacks'. */
    UINT                           uDDIVersion;              /* Direct3D DDI version which this device provides. */
    UINT                           uCreateDeviceFlags;       /* Requested pipeline level and threading. */
    D3DDDI_DEVICECALLBACKS const  *pRTCallbacks;             /* Direct3D runtime functions that the driver can use to access kernel services. */
    DXGI_DDI_BASE_CALLBACKS const *pDXGIBaseCallbacks;       /* Direct3D runtime functions that the driver can use to access kernel services. */

    D3D10DDI_HRTCORELAYER          hRTCoreLayer;             /* Handle to use when calling 'pUMCallbacks'. */
    D3D11DDI_CORELAYER_DEVICECALLBACKS const *pUMCallbacks;  /* Direct3D 11 runtime functions which the driver can use to access core user-mode runtime functionality. */

    /* VBoxDX device data. */
    PVBOXDXADAPTER            pAdapter;                      /* Adapter which created this device. */

    /* Miniport context. */
    HANDLE                    hContext;
    VOID                      *pCommandBuffer;
    UINT                      CommandBufferSize;
    D3DDDI_ALLOCATIONLIST     *pAllocationList;
    UINT                      AllocationListSize;
    D3DDDI_PATCHLOCATIONLIST  *pPatchLocationList;
    UINT                      PatchLocationListSize;

    UINT                      cbCommandBuffer;               /* Size of commands in the buffer */
    UINT                      cbCommandReserved;             /* Size of the current command. */
    UINT                      cAllocations;                  /* Number of allocation in pAllocationList. */
    UINT                      cPatchLocations;               /* Number of patch locations. */

    /* Handle tables for various objects. */
    RTHANDLETABLE hHTBlendState;
    RTHANDLETABLE hHTDepthStencilState;
    RTHANDLETABLE hHTRasterizerState;
    RTHANDLETABLE hHTSamplerState;
    RTHANDLETABLE hHTElementLayout;
    RTHANDLETABLE hHTShader;
    RTHANDLETABLE hHTShaderResourceView;
    RTHANDLETABLE hHTRenderTargetView;
    RTHANDLETABLE hHTDepthStencilView;
    RTHANDLETABLE hHTQuery;
    RTHANDLETABLE hHTUnorderedAccessView;
    RTHANDLETABLE hHTStreamOutput;

    /* Resources */
    RTLISTANCHOR                listResources;              /* All resources of this device, for cleanup. */
    RTLISTANCHOR                listDestroyedResources;     /* DestroyResource adds to this list. Flush actually deleted them. */
    RTLISTANCHOR                listStagingResources;       /* List of staging resources for uploads. */

    /* Shaders */
    D3DKMT_HANDLE               hShaderAllocation;          /* Shader allocation for this context. */
    uint32_t                    cbShaderAllocation;         /* Size of the shader's allocation: SVGA3D_MAX_SHADER_MEMORY_BYTES. */
    uint32_t                    cbShaderAvailable;          /* How many bytes is free in the shader's allocation. */
    uint32_t                    offShaderFree;              /* Offset of the next free byte in the shader's allocation. */
    RTLISTANCHOR                listShaders;                /* All shaders of this device, to be able to repack them in the shaders allocation. */

    /* Queries */
    RTLISTANCHOR                listQueries;                /* All queries of this device, to be able to repack them in the allocation. */
    RTLISTANCHOR                listCOAQuery;               /* List of VBOXDXCOALLOCATION for all query types. */
    uint64_t                    u64MobFenceValue;

    /* Stream output declarations */
    RTLISTANCHOR                listCOAStreamOutput;        /* List of VBOXDXCOALLOCATION for stream output declarations. */

    /* Pipeline state. */
    struct
    {
        uint32_t                cRenderTargetViews;
        PVBOXDXRENDERTARGETVIEW apRenderTargetViews[SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS];

        PVBOXDXDEPTHSTENCILVIEW pDepthStencilView;

        uint32_t                cUnorderedAccessViews;

        VBOXDXCONSTANTBUFFERSSTATE aConstantBuffers[SVGA3D_SHADERTYPE_MAX - SVGA3D_SHADERTYPE_MIN]; /* For each shader type. */
        VBOXDXVERTEXBUFFERSSTATE   VertexBuffers;
        VBOXDXINDEXBUFFERSTATE     IndexBuffer;
    } pipeline;
} VBOXDX_DEVICE, *PVBOXDX_DEVICE;


HRESULT vboxDXDeviceInit(PVBOXDX_DEVICE pDevice);
void vboxDXDestroyDevice(PVBOXDX_DEVICE pDevice);

HRESULT vboxDXDeviceFlushCommands(PVBOXDX_DEVICE pDevice);
void *vboxDXCommandBufferReserve(PVBOXDX_DEVICE pDevice, SVGAFifo3dCmdId enmCmd, uint32_t cbCmd, uint32_t cPatchLocations = 0);
void vboxDXCommandBufferCommit(PVBOXDX_DEVICE pDevice);

void vboxDXStorePatchLocation(PVBOXDX_DEVICE pDevice, void *pvPatch, VBOXDXALLOCATIONTYPE enmAllocationType,
                              D3DKMT_HANDLE hAllocation, uint32_t offAllocation, bool fWriteOperation);

SVGA3dSurfaceFormat vboxDXDxgiToSvgaFormat(DXGI_FORMAT enmDxgiFormat);

int vboxDXInitResourceData(PVBOXDX_RESOURCE pResource, const D3D11DDIARG_CREATERESOURCE *pCreateResource);
bool vboxDXCreateResource(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pResource,
                          const D3D11DDIARG_CREATERESOURCE *pCreateResource);
bool vboxDXOpenResource(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pResource,
                        const D3D10DDIARG_OPENRESOURCE *pOpenResource);
void vboxDXDestroyResource(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pResource);

HRESULT vboxDXRotateResourceIdentities(PVBOXDX_DEVICE pDevice, UINT cResources, PVBOXDX_RESOURCE *papResources);
HRESULT vboxDXOfferResources(PVBOXDX_DEVICE pDevice, UINT cResources, PVBOXDX_RESOURCE *papResources, D3DDDI_OFFER_PRIORITY Priority);
HRESULT vboxDXReclaimResources(PVBOXDX_DEVICE pDevice, UINT cResources, PVBOXDX_RESOURCE *papResources, BOOL *paDiscarded);

void vboxDXCreateBlendState(PVBOXDX_DEVICE pDevice, PVBOXDX_BLENDSTATE pBlendState);
void vboxDXDestroyBlendState(PVBOXDX_DEVICE pDevice, PVBOXDX_BLENDSTATE pBlendState);
void vboxDXCreateDepthStencilState(PVBOXDX_DEVICE pDevice, PVBOXDX_DEPTHSTENCIL_STATE pDepthStencilState);
void vboxDXDestroyDepthStencilState(PVBOXDX_DEVICE pDevice, PVBOXDX_DEPTHSTENCIL_STATE pDepthStencilState);
void vboxDXCreateRasterizerState(PVBOXDX_DEVICE pDevice, PVBOXDX_RASTERIZER_STATE pRasterizerState);
void vboxDXDestroyRasterizerState(PVBOXDX_DEVICE pDevice, PVBOXDX_RASTERIZER_STATE pRasterizerState);
void vboxDXCreateSamplerState(PVBOXDX_DEVICE pDevice, PVBOXDX_SAMPLER_STATE pSamplerState);
void vboxDXDestroySamplerState(PVBOXDX_DEVICE pDevice, PVBOXDX_SAMPLER_STATE pSamplerState);
void vboxDXCreateElementLayout(PVBOXDX_DEVICE pDevice, PVBOXDXELEMENTLAYOUT pElementLayout);
void vboxDXDestroyElementLayout(PVBOXDX_DEVICE pDevice, PVBOXDXELEMENTLAYOUT pElementLayout);
void vboxDXSetInputLayout(PVBOXDX_DEVICE pDevice, PVBOXDXELEMENTLAYOUT pInputLayout);
void vboxDXSetBlendState(PVBOXDX_DEVICE pDevice, PVBOXDX_BLENDSTATE pBlendState,
                         const FLOAT BlendFactor[4], UINT SampleMask);
void vboxDXSetDepthStencilState(PVBOXDX_DEVICE pDevice, PVBOXDX_DEPTHSTENCIL_STATE pDepthStencilState, UINT StencilRef);
void vboxDXSetRasterizerState(PVBOXDX_DEVICE pDevice, PVBOXDX_RASTERIZER_STATE pRasterizerState);
void vboxDXSetSamplers(PVBOXDX_DEVICE pDevice, SVGA3dShaderType enmShaderType,
                       UINT StartSlot, UINT NumSamplers, const uint32_t *paSamplerIds);
void vboxDXIaSetTopology(PVBOXDX_DEVICE pDevice, D3D10_DDI_PRIMITIVE_TOPOLOGY PrimitiveTopology);
void vboxDXDrawIndexed(PVBOXDX_DEVICE pDevice, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
void vboxDXDraw(PVBOXDX_DEVICE pDevice, UINT VertexCount, UINT StartVertexLocation);
void vboxDXDrawIndexedInstanced(PVBOXDX_DEVICE pDevice, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
void vboxDXDrawInstanced(PVBOXDX_DEVICE pDevice, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
void vboxDXDrawAuto(PVBOXDX_DEVICE pDevice);
void vboxDXSetViewports(PVBOXDX_DEVICE pDevice, UINT NumViewports, UINT ClearViewports, const D3D10_DDI_VIEWPORT *pViewports);
void vboxDXSetScissorRects(PVBOXDX_DEVICE pDevice, UINT NumRects, UINT ClearRects, const D3D10_DDI_RECT *pRects);
void vboxDXCreateShader(PVBOXDX_DEVICE pDevice, SVGA3dShaderType enmShaderType, PVBOXDXSHADER pShader, const UINT* pShaderCode,
                        const D3D11_1DDIARG_SIGNATURE_ENTRY2* pInputSignature, UINT NumInputSignatureEntries,
                        const D3D11_1DDIARG_SIGNATURE_ENTRY2* pOutputSignature, UINT NumOutputSignatureEntries,
                        const D3D11_1DDIARG_SIGNATURE_ENTRY2* pPatchConstantSignature, UINT NumPatchConstantSignatureEntries);
void vboxDXCreateStreamOutput(PVBOXDX_DEVICE pDevice, PVBOXDXSHADER pShader,
                              const D3D11DDIARG_STREAM_OUTPUT_DECLARATION_ENTRY *pOutputStreamDecl, UINT NumEntries,
                              const UINT  *BufferStridesInBytes, UINT NumStrides,
                              UINT RasterizedStream);
void vboxDXDestroyShader(PVBOXDX_DEVICE pDevice, PVBOXDXSHADER pShader);
void vboxDXCreateQuery(PVBOXDX_DEVICE pDevice, PVBOXDXQUERY pQuery, D3D10DDI_QUERY Query, UINT MiscFlags);
void vboxDXDestroyQuery(PVBOXDX_DEVICE pDevice, PVBOXDXQUERY pQuery);
void vboxDXQueryBegin(PVBOXDX_DEVICE pDevice, PVBOXDXQUERY pQuery);
void vboxDXQueryEnd(PVBOXDX_DEVICE pDevice, PVBOXDXQUERY pQuery);
void vboxDXQueryGetData(PVBOXDX_DEVICE pDevice, PVBOXDXQUERY pQuery, VOID* pData, UINT DataSize, UINT Flags);
void vboxDXSetPredication(PVBOXDX_DEVICE pDevice, PVBOXDXQUERY pQuery, BOOL PredicateValue);
void vboxDXSetShader(PVBOXDX_DEVICE pDevice, SVGA3dShaderType enmShaderType, PVBOXDXSHADER pShader);
void vboxDXSetVertexBuffers(PVBOXDX_DEVICE pDevice, UINT StartSlot, UINT NumBuffers,
                            PVBOXDX_RESOURCE *papBuffers, const UINT *pStrides, const UINT *pOffsets);
void vboxDXSetIndexBuffer(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pBuffer, DXGI_FORMAT Format, UINT Offset);
void vboxDXResourceUpdateSubresourceUP(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pDstResource,
                                       UINT DstSubresource, const D3D10_DDI_BOX *pDstBox,
                                       const VOID *pSysMemUP, UINT RowPitch, UINT DepthPitch, UINT CopyFlags);
void vboxDXResourceMap(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pResource, UINT Subresource,
                              D3D10_DDI_MAP DDIMap, UINT Flags, D3D10DDI_MAPPED_SUBRESOURCE *pMappedSubResource);
void vboxDXResourceUnmap(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pResource, UINT Subresource);
void vboxDXSoSetTargets(PVBOXDX_DEVICE pDevice, uint32_t NumTargets, D3DKMT_HANDLE *paAllocations, uint32_t *paOffsets, uint32_t *paSizes);
void vboxDXCreateShaderResourceView(PVBOXDX_DEVICE pDevice, PVBOXDXSHADERRESOURCEVIEW pShaderResourceView);
void vboxDXGenMips(PVBOXDX_DEVICE pDevice, PVBOXDXSHADERRESOURCEVIEW pShaderResourceView);
void vboxDXDestroyShaderResourceView(PVBOXDX_DEVICE pDevice, PVBOXDXSHADERRESOURCEVIEW pShaderResourceView);
void vboxDXCreateRenderTargetView(PVBOXDX_DEVICE pDevice, PVBOXDXRENDERTARGETVIEW pRenderTargetView);
void vboxDXClearRenderTargetView(PVBOXDX_DEVICE pDevice, PVBOXDXRENDERTARGETVIEW pRenderTargetView, const FLOAT ColorRGBA[4]);
void vboxDXClearRenderTargetViewRegion(PVBOXDX_DEVICE pDevice, PVBOXDXRENDERTARGETVIEW pRenderTargetView, const FLOAT Color[4], const D3D10_DDI_RECT *pRect, UINT NumRects);
void vboxDXDestroyRenderTargetView(PVBOXDX_DEVICE pDevice, PVBOXDXRENDERTARGETVIEW pRenderTargetView);
void vboxDXCreateDepthStencilView(PVBOXDX_DEVICE pDevice, PVBOXDXDEPTHSTENCILVIEW pDepthStencilView);
void vboxDXClearDepthStencilView(PVBOXDX_DEVICE pDevice, PVBOXDXDEPTHSTENCILVIEW pDepthStencilView,
                                 UINT Flags, FLOAT Depth, UINT8 Stencil);
void vboxDXDestroyDepthStencilView(PVBOXDX_DEVICE pDevice, PVBOXDXDEPTHSTENCILVIEW pDepthStencilView);
void vboxDXSetRenderTargets(PVBOXDX_DEVICE pDevice, PVBOXDXDEPTHSTENCILVIEW pDepthStencilView,
                            uint32_t NumRTVs, UINT ClearSlots, PVBOXDXRENDERTARGETVIEW *papRenderTargetViews);
void vboxDXSetShaderResourceViews(PVBOXDX_DEVICE pDevice, SVGA3dShaderType enmShaderType, uint32_t StartSlot,
                                  uint32_t NumViews, uint32_t *paViewIds);
void vboxDXSetConstantBuffers(PVBOXDX_DEVICE pDevice, SVGA3dShaderType enmShaderType, UINT StartSlot, UINT NumBuffers,
                              PVBOXDX_RESOURCE *papBuffers, const UINT *pFirstConstant, const UINT *pNumConstants);
void vboxDXResourceCopyRegion(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pDstResource, UINT DstSubresource,
                              UINT DstX, UINT DstY, UINT DstZ, PVBOXDX_RESOURCE pSrcResource, UINT SrcSubresource,
                              const D3D10_DDI_BOX *pSrcBox, UINT CopyFlags);
void vboxDXResourceCopy(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pDstResource, PVBOXDX_RESOURCE pSrcResource);
void vboxDXCreateUnorderedAccessView(PVBOXDX_DEVICE pDevice, PVBOXDXUNORDEREDACCESSVIEW pUnorderedAccessView);
void vboxDXDestroyUnorderedAccessView(PVBOXDX_DEVICE pDevice, PVBOXDXUNORDEREDACCESSVIEW pUnorderedAccessView);
void vboxDXClearUnorderedAccessViewUint(PVBOXDX_DEVICE pDevice, PVBOXDXUNORDEREDACCESSVIEW pUnorderedAccessView, const UINT Values[4]);
void vboxDXClearUnorderedAccessViewFloat(PVBOXDX_DEVICE pDevice, PVBOXDXUNORDEREDACCESSVIEW pUnorderedAccessView, const FLOAT Values[4]);
void vboxDXSetUnorderedAccessViews(PVBOXDX_DEVICE pDevice, UINT StartSlot, UINT NumViews, const PVBOXDXUNORDEREDACCESSVIEW *papViews, const UINT *pUAVInitialCounts);
void vboxDXCsSetUnorderedAccessViews(PVBOXDX_DEVICE pDevice, UINT StartSlot, UINT NumViews, const uint32_t *paViewIds, const UINT* pUAVInitialCounts);
void vboxDXDispatch(PVBOXDX_DEVICE pDevice, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
void vboxDXDispatchIndirect(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pResource, UINT AlignedByteOffsetForArgs);
void vboxDXDrawIndexedInstancedIndirect(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pResource, UINT AlignedByteOffsetForArgs);
void vboxDXDrawInstancedIndirect(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pResource, UINT AlignedByteOffsetForArgs);
void vboxDXCopyStructureCount(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pDstBuffer, UINT DstAlignedByteOffset, PVBOXDXUNORDEREDACCESSVIEW pSrcView);
HRESULT vboxDXBlt(PVBOXDX_DEVICE pDevice, PVBOXDX_RESOURCE pDstResource, UINT DstSubresource,
                  PVBOXDX_RESOURCE pSrcResource, UINT SrcSubresource,
                  UINT DstLeft, UINT DstTop, UINT DstRight, UINT DstBottom,
                  DXGI_DDI_ARG_BLT_FLAGS Flags, DXGI_DDI_MODE_ROTATION Rotate);
HRESULT vboxDXFlush(PVBOXDX_DEVICE pDevice, bool fForce);

DECLINLINE(void) vboxDXDeviceSetError(PVBOXDX_DEVICE pDevice, HRESULT hr)
{
    Assert(hr == DXGI_DDI_ERR_WASSTILLDRAWING);
    pDevice->pUMCallbacks->pfnSetErrorCb(pDevice->hRTCoreLayer, hr);
}

DECLINLINE(D3DKMT_HANDLE) vboxDXGetAllocation(PVBOXDX_RESOURCE pResource)
{
    return pResource ? pResource->pKMResource->hAllocation : 0;
}

#endif /* !GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_wddm_dx_VBoxDX_h */
