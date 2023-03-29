// Fill out your copyright notice in the Description page of Project Settings.

#include "ImportOBJActor.h"
#include <StaticMeshAttributes.h>
#include <MeshDescriptionBuilder.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "Learning/tiny_obj_loader.h"
#include "Learning/lodepng.h"

DEFINE_LOG_CATEGORY_STATIC(LogImportOBJActor, All, All);

AImportOBJActor::AImportOBJActor() {
    PrimaryActorTick.bCanEverTick = false;

    // 初始化静态网格体组件
    _mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(meshName));
    SetRootComponent(_mesh);
    // 设置网格体
    _mesh->SetStaticMesh(CreateMeshDataFromFile(filePathRoot, objFileName));
    _mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
    
    // 将创建的纹理 赋值给 网格体的材质
    for (std::map<FPolygonGroupID, std::string>::iterator it = _materialIdMap.begin(); it != _materialIdMap.end(); ++it) {
        UMaterialInstanceDynamic* mtl = _materialMap[it->second];
        _mesh->SetMaterial(it->first, mtl);
    }
    UE_LOG(LogImportOBJActor, Display, TEXT("--!-- 材质构建成功 --!--"));
}

void AImportOBJActor::BeginPlay() {
    Super::BeginPlay();
}

void AImportOBJActor::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}

// 创建网格体数据
UStaticMesh* AImportOBJActor::CreateMeshDataFromFile(const FString& baseDir, const FString& file) {
    // 创建 UStaticMesh
    UStaticMesh* staticMesh = NewObject<UStaticMesh>(this, "UserMesh");
    FMeshDescription description;
    FStaticMeshAttributes attributes(description);
    attributes.Register();

    // 使用 tinyobj 读取文件内容
    tinyobj::attrib_t attrib;                   // OBJ文件中的顶点属性数组
    std::vector<tinyobj::shape_t> shapes;       // OBJ文件中的图元组(UE中的多边形组), 每个图元组对应一种材质
    std::vector<tinyobj::material_t> materials; // OBJ文件中的材质
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, TCHAR_TO_UTF8(*(baseDir + file)), TCHAR_TO_UTF8(*baseDir), true);
    if (!ret) {
        UE_LOG(LogImportOBJActor, Error, TEXT("tinyobj::LoadObj 失败, 提示信息为: %s"), UTF8_TO_TCHAR(err.c_str()));
        return staticMesh;
    } else {
        UE_LOG(LogImportOBJActor, Display, TEXT("tinyobj::LoadObj 成功, 路径为: %s"), *(baseDir + file));
    }
    for (int i = 0; i < materials.size(); i++) {
        UE_LOG(LogImportOBJActor, Display, TEXT("第%d个材质为: [%s]"), i, UTF8_TO_TCHAR(materials[i].name.c_str()));
    }

    // 使用 MeshDescriptionBuilder 设置网格模型的基本属性
    FMeshDescriptionBuilder meshDescBuilder;
    meshDescBuilder.SetMeshDescription(&description);   // 设置 MeshDescription
    meshDescBuilder.EnablePolyGroups();                 // 允许使用多边形组
    meshDescBuilder.SetNumUVLayers(1);                  // 设置UV坐标的级数为1

    // 向 MeshDescriptionBuilder 中输入顶点位置
    TArray<FVertexID> vertexIDs;
    vertexIDs.SetNum(attrib.vertices.size() / 3);
    UE_LOG(LogImportOBJActor, Display, TEXT("第一个顶点的信息: %f, %f, %f, %f, %f, %f"), 
        attrib.vertices[0], attrib.vertices[1], attrib.vertices[2], 
        attrib.vertices[3], attrib.vertices[4], attrib.vertices[5]);
    for (int v = 0; v < vertexIDs.Num(); v++) {
        vertexIDs[v] = meshDescBuilder.AppendVertex(
            FVector(attrib.vertices[3 * v + 0], attrib.vertices[3 * v + 1], attrib.vertices[3 * v + 2])
        );
    }
    UE_LOG(LogImportOBJActor, Display, TEXT("导入顶点位置成功, 共导入 %d 个顶点"), attrib.vertices.size() / 3);

    // 遍历所有的图元对象, 创建多边形组
    for (size_t i = 0; i < shapes.size(); ++i) {
        // 当前多边形组
        tinyobj::shape_t& shape = shapes[i];

        // 找到与该多边形组相关的材质
        tinyobj::material_t material;
        if (!shape.mesh.material_ids.empty()) {
            int mtlID = shape.mesh.material_ids[0];
            if (mtlID >= 0) material = materials[mtlID];
            UE_LOG(LogImportOBJActor, Display, TEXT("[第%d个多边形组] 对应材质为 %s"), i, UTF8_TO_TCHAR(material.name.c_str()));
        } else {
            UE_LOG(LogImportOBJActor, Warning, TEXT("[第%d个多边形组] 没有材质"), i);
        }

        // 初始化 GlobalData, 用于后续传递参数
        GlobalData globalData;
        globalData.builder = &meshDescBuilder;
        globalData.vertexIDs = vertexIDs;
        globalData.polygonGroup = meshDescBuilder.AppendPolygonGroup();

        // 设置材质映射, 并且根据需要创建动态材质实例
        _materialIdMap[globalData.polygonGroup] = material.name;
        if (_materialMap.find(material.name) == _materialMap.end()) {
            // 找到已经创建好的UE材质资产(只能在构造函数中使用FObjectFinder): /Game/BasicTexture
            static ConstructorHelpers::FObjectFinder<UMaterial> mtlAsset(*materialPath);
            // 创建动态材质实例
            if (mtlAsset.Succeeded()) {
                UE_LOG(LogImportOBJActor, Display, TEXT("[第%d个多边形组] 纹理为: %s"), i, UTF8_TO_TCHAR(material.diffuse_texname.c_str()));
                UMaterialInstanceDynamic* mtl = UMaterialInstanceDynamic::Create(mtlAsset.Object, NULL);
                UTexture2D* diffuseTexture2D = CreateTexture(baseDir, UTF8_TO_TCHAR(material.diffuse_texname.c_str()));
                if (diffuseTexture2D) {
                    mtl->SetTextureParameterValue("BaseTexture", diffuseTexture2D);
                    _materialMap[material.name] = mtl;
                }
            }
        }

        // 遍历 shape.mesh 中的所有三角面, 并将三角面信息添加到 MeshDescriptionBuilder 中
        size_t offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            size_t fnum = shape.mesh.num_face_vertices[f];
            if (fnum != 3) continue;

            // 读取该三角面中三个顶点的属性
            TriangleVertex triVertex[3];
            for (size_t v = 0; v < 3; ++v) {
                // 顶点编号
                tinyobj::index_t idx = shape.mesh.indices[offset + v];
                triVertex[v].vertexIndex = idx.vertex_index;
                // 顶点法线
                if (idx.normal_index >= 0) {
                    triVertex[v].normal = FVector(
                        -attrib.normals[3 * idx.normal_index + 0],
                        -attrib.normals[3 * idx.normal_index + 1], 
                        -attrib.normals[3 * idx.normal_index + 2]);
                }
                // 顶点UV坐标(-!--注意要翻转Y轴--!-)
                if (idx.texcoord_index >= 0) {
                    triVertex[v].UV = FVector2D(
                        attrib.texcoords[2 * idx.texcoord_index + 0], 
                        1.0 - attrib.texcoords[2 * idx.texcoord_index + 1]);
                } else {
                    triVertex[v].UV = FVector2D(0.0, 0.0);
                }
            }
            // 将三个顶点的属性添加到 MeshDescriptionBuilder 中
            AddTriangleData(globalData, triVertex[0], triVertex[1], triVertex[2]);
            offset += fnum;
        }

        // 提示信息
        UE_LOG(LogImportOBJActor, Display, TEXT("多边形组 %s 添加成功, 对应材质为: %s"), UTF8_TO_TCHAR(shape.name.c_str()), UTF8_TO_TCHAR(material.name.c_str()));
    }

    // 创建 MeshDescription
    UStaticMesh::FBuildMeshDescriptionsParams builderParams;
    builderParams.bBuildSimpleCollision = true;
    builderParams.bFastBuild = true;

    // 根据 MeshDescription 构建网格体
    TArray<const FMeshDescription*> meshDescList;
    meshDescList.Emplace(&description);
    staticMesh->BuildFromMeshDescriptions(meshDescList, builderParams);

    // 返回构建成功的网格体
    UE_LOG(LogImportOBJActor, Display, TEXT("--!-- 网格体构建成功 --!--"));
    return staticMesh;
}

// 读取磁盘上的PNG图片, 创建纹理
UTexture2D* AImportOBJActor::CreateTexture(const FString& baseDir, const FString& file) {
    // 读取磁盘上的png图片
    std::vector<unsigned char> data;
    unsigned int w = 0, h = 0;
    unsigned int result = lodepng::decode(data, w, h, TCHAR_TO_UTF8(*(baseDir + file)));
    if (result > 0) {
        UE_LOG(LogTemp, Error, TEXT("[lodepng]读取PNG文件 %s 失败, 错误信息为: %s"), *(baseDir + file), UTF8_TO_TCHAR(lodepng_error_text(result)));
        return NULL;
    }

    // 创建新的UTexture2D对象
    UTexture2D* texture2D = UTexture2D::CreateTransient(w, h, PF_R8G8B8A8);
    void* texData = texture2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    if (texData) {
        FMemory::Memcpy(texData, &data[0], data.size());
        UE_LOG(LogTemp, Display, TEXT("纹理 %s 创建成功"), *file);
    } else {
        UE_LOG(LogTemp, Error, TEXT("纹理 %s 创建失败"), *file);
    }
    texture2D->PlatformData->Mips[0].BulkData.Unlock();
    texture2D->UpdateResource();
    return texture2D;
}

// 向几何体中添加三角面信息
void AImportOBJActor::AddTriangleData(
    GlobalData& globalData, const TriangleVertex& v1, const TriangleVertex& v2, const TriangleVertex& v3) {

    // 新建三个 VertexInstance, 设置对应的顶点属性
    FVertexInstanceID i1 = globalData.builder->AppendInstance(globalData.vertexIDs[v1.vertexIndex]);
    globalData.builder->SetInstanceNormal(i1, v1.normal);
    globalData.builder->SetInstanceUV(i1, v1.UV, 0);
    globalData.builder->SetInstanceColor(i1, v1.Color);

    FVertexInstanceID i2 = globalData.builder->AppendInstance(globalData.vertexIDs[v2.vertexIndex]);
    globalData.builder->SetInstanceNormal(i2, v2.normal);
    globalData.builder->SetInstanceUV(i2, v2.UV, 0);
    globalData.builder->SetInstanceColor(i2, v2.Color);

    FVertexInstanceID i3 = globalData.builder->AppendInstance(globalData.vertexIDs[v3.vertexIndex]);
    globalData.builder->SetInstanceNormal(i3, v3.normal);
    globalData.builder->SetInstanceUV(i3, v3.UV, 0);
    globalData.builder->SetInstanceColor(i3, v3.Color);

    // 最后, 通过三个索引值构建三角面图元
    globalData.builder->AppendTriangle(i1, i2, i3, globalData.polygonGroup);
}