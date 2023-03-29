// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <map>
#include "ImportOBJActor.generated.h"

class FMeshDescriptionBuilder;

UCLASS()
class LEARNING_API AImportOBJActor : public AActor {
    GENERATED_BODY()

public:
    AImportOBJActor();

    UPROPERTY(EditAnywhere, meta = (ToolTip = "待导入的Mesh在场景中的名称"))
    FString meshName = "Mesh";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "导入OBJ文件的路径"))
    FString filePathRoot = "D://Default//Desktop//OutputOBJ//";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "导入OBJ文件的名称"))
    FString objFileName = "result_ryota.obj";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "材质资产路径"))
    FString materialPath = "Material '/Game/BasicTexture.BasicTexture'";

    // 静态网格体组件
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* _mesh;

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

private:
    // 当前参与构建几何体的主要对象
    struct GlobalData {
        FMeshDescriptionBuilder* builder;   // MeshDescriptionBuilder 对象
        TArray<FVertexID> vertexIDs;        // 顶点ID
        FPolygonGroupID polygonGroup;       // 多边形组
    };
    // 三角形顶点实例数据
    struct TriangleVertex {
        int vertexIndex;    // 顶点编号
        FVector normal;     // 顶点法线
        FVector2D UV;       // 顶点UV贴图
        FVector4f Color = FVector4f(1.0f, 1.0f, 1.0f, 1.0f); // 顶点颜色
    };

    // 映射: 多边形组 => 材质名称
    std::map<FPolygonGroupID, std::string> _materialIdMap;
    // 映射: 材质名称 => 材质实例
    std::map<std::string, UMaterialInstanceDynamic*> _materialMap;

private:
    // 创建网格体数据
    UStaticMesh* CreateMeshDataFromFile(const FString& baseDir, const FString& file);
    // 读取磁盘上的PNG图片, 创建纹理
    UTexture2D* CreateTexture(const FString& baseDir, const FString& file);
    // 向几何体中添加三角面信息
    void AddTriangleData(GlobalData& globalData, const TriangleVertex& v1, const TriangleVertex& v2, const TriangleVertex& v3);
};
