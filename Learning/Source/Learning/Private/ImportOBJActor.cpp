// Fill out your copyright notice in the Description page of Project Settings.

#include "ImportOBJActor.h"
#include <StaticMeshAttributes.h>
#include <MeshDescriptionBuilder.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "Learning/tiny_obj_loader.h"
#include "Learning/lodepng.h"

AImportOBJActor::AImportOBJActor() {
    PrimaryActorTick.bCanEverTick = false;

    // 初始化静态网格体组件
    _mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(meshName));
    SetRootComponent(_mesh);

    // 设置网格体
    _mesh->SetStaticMesh(CreateMeshDataFromFile(filePathRoot, objFileName));
    _mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
    
    // 设置材质
    for (std::map<FPolygonGroupID, std::string>::iterator it = _materialIdMap.begin(); it != _materialIdMap.end(); ++it) {
        UMaterialInstanceDynamic* mtl = _materialMap[it->second];
        _mesh->SetMaterial(it->first, mtl);
    }
}

void AImportOBJActor::BeginPlay() {
    Super::BeginPlay();
}

void AImportOBJActor::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}

// 创建网格体数据
UStaticMesh* AImportOBJActor::CreateMeshDataFromFile(const FString& baseDir, const FString& file) {
    // 创建UStaticMesh
    UStaticMesh* staticMesh = NewObject<UStaticMesh>(this, "UserMesh");
    FMeshDescription meshDesc;
    FStaticMeshAttributes attributes(meshDesc);
    attributes.Register();

    // 使用tinyobj读取文件内容
    tinyobj::attrib_t attrib;                   // OBJ文件中的顶点属性数组
    std::vector<tinyobj::shape_t> shapes;       //
    std::vector<tinyobj::material_t> materials;
    std::string err;
    bool ret = tinyobj::LoadObj(
        &attrib, &shapes, &materials, &err, 
        TCHAR_TO_UTF8(*(baseDir + file)), 
        TCHAR_TO_UTF8(*baseDir), true);
    
    if (ret) {
        // 将顶点位置坐标数组输入到 meshDescBuilder
        FMeshDescriptionBuilder meshDescBuilder;
        meshDescBuilder.SetMeshDescription(&meshDesc);
        meshDescBuilder.EnablePolyGroups();
        meshDescBuilder.SetNumUVLayers(1);
        // 创建顶点数组
        TArray<FVertexID> vertexIDs;
        vertexIDs.SetNum(attrib.vertices.size() / 3);
        for (int v = 0; v < vertexIDs.Num(); ++v) {
            vertexIDs[v] =
                meshDescBuilder.AppendVertex(FVector(
                    attrib.vertices[3 * v + 0], 
                    attrib.vertices[3 * v + 1], 
                    attrib.vertices[3 * v + 2]));
        }

        // 遍历所有的图元对象, 创建多边形组
        for (size_t i = 0; i < shapes.size(); ++i) {
            // 找到与该多边形组相关的材质
            tinyobj::shape_t& shape = shapes[i];
            tinyobj::material_t material;
            if (!shape.mesh.material_ids.empty()) {
                int mtlID = shape.mesh.material_ids[0];
                if (mtlID >= 0) material = materials[mtlID];
            }

            GlobalData globalData;
            globalData.builder = &meshDescBuilder;
            globalData.vertexIDs = vertexIDs;
            globalData.polygonGroup = meshDescBuilder.AppendPolygonGroup();

            // Set material map and create material if required
            _materialIdMap[globalData.polygonGroup] = material.name;
            if (_materialMap.find(material.name) == _materialMap.end()) {
                static ConstructorHelpers::FObjectFinder<UMaterial> mtlAsset(TEXT("Material'/Game/BasicTexture.BasicTexture'"));
                if (mtlAsset.Succeeded()) {
                    UMaterialInstanceDynamic* mtl = UMaterialInstanceDynamic::Create(mtlAsset.Object, NULL);
                    mtl->SetTextureParameterValue("BaseTexture", CreateTexture(baseDir, UTF8_TO_TCHAR(material.diffuse_texname.c_str())));
                    _materialMap[material.name] = mtl;
                }
            }

            // 添加三角面信息
            size_t offset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
                size_t fnum = shape.mesh.num_face_vertices[f];
                if (fnum != 3) continue;

                TriangleVertex triVertex[3];
                for (size_t v = 0; v < 3; ++v) {
                    tinyobj::index_t idx = shape.mesh.indices[offset + v];
                    triVertex[v].vertexIndex = idx.vertex_index;

                    if (idx.normal_index >= 0) {
                        triVertex[v].instanceNormal = FVector(
                            -attrib.normals[3 * idx.normal_index + 0],
                            -attrib.normals[3 * idx.normal_index + 1], 
                            -attrib.normals[3 * idx.normal_index + 2]);
                    }

                    if (idx.texcoord_index >= 0) {
                        triVertex[v].instanceUV = FVector2D(
                            attrib.texcoords[2 * idx.texcoord_index + 0], 
                            1.0 - attrib.texcoords[2 * idx.texcoord_index + 1]);
                    } else {
                        triVertex[v].instanceUV = FVector2D(0.0, 0.0);
                    }
                }
                AddTriangleData(globalData, triVertex[0], triVertex[1], triVertex[2]);
                offset += fnum;
            }
            UE_LOG(LogTemp, Warning, TEXT("Add shape: %s (MTL = %s)"), UTF8_TO_TCHAR(shape.name.c_str()), UTF8_TO_TCHAR(material.name.c_str()));
        }

        // Build the mesh description
        UStaticMesh::FBuildMeshDescriptionsParams builderParams;
        builderParams.bBuildSimpleCollision = true;
        builderParams.bFastBuild = true;

        TArray<const FMeshDescription*> meshDescList;
        meshDescList.Emplace(&meshDesc);
        staticMesh->BuildFromMeshDescriptions(meshDescList, builderParams);
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load OBJ file: %s"), UTF8_TO_TCHAR(err.c_str()));
    }
    return staticMesh;
}

// 创建纹理
UTexture2D* AImportOBJActor::CreateTexture(const FString& baseDir, const FString& file) {
    // 读取磁盘上的png图片
    std::vector<unsigned char> data;
    unsigned int w = 0, h = 0;
    unsigned int result = lodepng::decode(data, w, h, TCHAR_TO_UTF8(*(baseDir + file)));
    if (result > 0) {
        UE_LOG(LogTemp, Warning, TEXT("lodepng: %s"), UTF8_TO_TCHAR(lodepng_error_text(result)));
        return NULL;
    }

    // 创建新的UTexture2D对象
    UTexture2D* tex2D = UTexture2D::CreateTransient(w, h, PF_R8G8B8A8);
    void* texData = tex2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    if (texData) {
        FMemory::Memcpy(texData, &data[0], data.size());
        UE_LOG(LogTemp, Warning, TEXT("Texture created: %s"), *file);
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Texture is invalid for %s"), *file);
    }
    tex2D->PlatformData->Mips[0].BulkData.Unlock();
    tex2D->UpdateResource();
    return tex2D;
}

// 向几何体中添加三角面信息
void AImportOBJActor::AddTriangleData(
    GlobalData& globalData, const TriangleVertex& v1, const TriangleVertex& v2, const TriangleVertex& v3) {

    FVertexInstanceID i1 = globalData.builder->AppendInstance(globalData.vertexIDs[v1.vertexIndex]);
    globalData.builder->SetInstanceNormal(i1, v1.instanceNormal);
    globalData.builder->SetInstanceUV(i1, v1.instanceUV, 0);
    globalData.builder->SetInstanceColor(i1, FVector4f(1.0f, 1.0f, 1.0f, 1.0f));

    FVertexInstanceID i2 = globalData.builder->AppendInstance(globalData.vertexIDs[v2.vertexIndex]);
    globalData.builder->SetInstanceNormal(i2, v2.instanceNormal);
    globalData.builder->SetInstanceUV(i2, v2.instanceUV, 0);
    globalData.builder->SetInstanceColor(i2, FVector4f(1.0f, 1.0f, 1.0f, 1.0f));

    FVertexInstanceID i3 = globalData.builder->AppendInstance(globalData.vertexIDs[v3.vertexIndex]);
    globalData.builder->SetInstanceNormal(i3, v3.instanceNormal);
    globalData.builder->SetInstanceUV(i3, v3.instanceUV, 0);
    globalData.builder->SetInstanceColor(i3, FVector4f(1.0f, 1.0f, 1.0f, 1.0f));

    globalData.builder->AppendTriangle(i1, i2, i3, globalData.polygonGroup);
}