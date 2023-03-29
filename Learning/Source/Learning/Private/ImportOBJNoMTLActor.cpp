// Fill out your copyright notice in the Description page of Project Settings.

#include "ImportOBJNoMTLActor.h"
#include <StaticMeshAttributes.h>
#include <MeshDescriptionBuilder.h>
#include "Learning/tiny_obj_loader.h"

DEFINE_LOG_CATEGORY_STATIC(LogImportOBJNoSTLActor, All, All);

AImportOBJNoMTLActor::AImportOBJNoMTLActor() {
    PrimaryActorTick.bCanEverTick = false;
    // 初始化静态网格体组件
    _mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(meshName));
    SetRootComponent(_mesh);
}

void AImportOBJNoMTLActor::BeginPlay() {
    Super::BeginPlay();
    // 设置网格体
    _mesh->SetStaticMesh(CreateMeshDataFromFile(filePathRoot, objFileName));
    _mesh->SetRelativeLocation(Location);
    _mesh->SetRelativeRotation_Direct(Rotation);
}

void AImportOBJNoMTLActor::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}

UStaticMesh* AImportOBJNoMTLActor::CreateMeshDataFromFile(const FString& baseDir, const FString& file) {
    // 创建 UStaticMesh
    UStaticMesh* staticMesh = NewObject<UStaticMesh>(this, "UserMesh");
    FMeshDescription description;
    FStaticMeshAttributes attributes(description);
    attributes.Register();

    // 使用 tinyobj 读取文件内容
    tinyobj::attrib_t attrib;                    // OBJ文件中的顶点属性数组
    std::vector<tinyobj::shape_t> shapes;        // OBJ文件中的图元组(UE中的多边形组), 每个图元组对应一种材质
    std::vector<tinyobj::material_t> materials;  // OBJ文件中的材质
    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, TCHAR_TO_UTF8(*(baseDir + file)), TCHAR_TO_UTF8(*baseDir), true, true);
    if (!ret) {
        UE_LOG(LogImportOBJNoSTLActor, Error, TEXT("tinyobj::LoadObj 失败, 提示信息为: %s"), UTF8_TO_TCHAR(err.c_str()));
        return staticMesh;
    } else {
        UE_LOG(LogImportOBJNoSTLActor, Display, TEXT("tinyobj::LoadObj 成功, 路径为: %s"), *(baseDir + file));
    }

    // 使用 MeshDescriptionBuilder 设置网格模型的基本属性
    FMeshDescriptionBuilder meshDescBuilder;
    meshDescBuilder.SetMeshDescription(&description);  // 设置 MeshDescription
    meshDescBuilder.EnablePolyGroups();                // 允许使用多边形组
    meshDescBuilder.SetNumUVLayers(1);                 // 设置UV坐标的级数为1

    // 向 MeshDescriptionBuilder 中输入顶点位置
    TArray<FVertexID> vertexIDs;
    vertexIDs.SetNum(attrib.vertices.size() / 3);
    for (int v = 0; v < vertexIDs.Num(); v++) {
        vertexIDs[v] = meshDescBuilder.AppendVertex(FVector(
            attrib.vertices[3 * v + 0] * 100.0f, 
            attrib.vertices[3 * v + 1] * 100.0f, 
            attrib.vertices[3 * v + 2] * 100.0f)
        );
    }
    UE_LOG(LogImportOBJNoSTLActor, Display, TEXT("导入顶点位置成功, 共导入 %d 个顶点"), attrib.vertices.size() / 3);

    // 遍历所有的图元对象, 创建多边形组
    for (size_t i = 0; i < shapes.size(); ++i) {
        // 当前多边形组
        tinyobj::shape_t& shape = shapes[i];

        // 初始化 GlobalData, 用于后续传递参数
        GlobalData globalData;
        globalData.builder = &meshDescBuilder;
        globalData.vertexIDs = vertexIDs;
        globalData.polygonGroup = meshDescBuilder.AppendPolygonGroup();

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
                        attrib.normals[3 * idx.normal_index + 0], 
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    );
                }
                
                // 顶点UV坐标(-!--注意要翻转Y轴--!-)
                if (idx.texcoord_index >= 0) {
                    triVertex[v].UV = FVector2D(
                        attrib.texcoords[2 * idx.texcoord_index + 0], 
                        1.0 - attrib.texcoords[2 * idx.texcoord_index + 1]
                    );
                } 
                
                // 顶点颜色
                if (attrib.colors.size() > 3 * idx.vertex_index + 2) {
                    triVertex[v].Color = FVector4f(
                        attrib.colors[3 * idx.vertex_index + 0], 
                        attrib.colors[3 * idx.vertex_index + 1],
                        attrib.colors[3 * idx.vertex_index + 2],
                        1.0f
                    );
                }
            }
            // 将三个顶点的属性添加到 MeshDescriptionBuilder 中
            AddTriangleData(globalData, triVertex[0], triVertex[1], triVertex[2]);
            offset += fnum;
        }

        // 提示信息
        UE_LOG(LogImportOBJNoSTLActor, Display, TEXT("多边形组 %s 添加成功"), UTF8_TO_TCHAR(shape.name.c_str()));
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
    UE_LOG(LogImportOBJNoSTLActor, Display, TEXT("--!-- 网格体构建成功 --!--"));
    return staticMesh;
}

void AImportOBJNoMTLActor::AddTriangleData(
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