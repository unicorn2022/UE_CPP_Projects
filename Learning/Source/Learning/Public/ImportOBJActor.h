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

    UPROPERTY(EditAnywhere, meta = (ToolTip = "a mesh name in the scene"))
    FString meshName = "Mesh";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "path root of the output file"))
    FString filePathRoot = "D://Default//Desktop//TestModels//";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "path root of the output file"))
    FString objFileName = "Minimal_Default.obj";

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
        FMeshDescriptionBuilder* builder;
        TArray<FVertexID> vertexIDs;
        FPolygonGroupID polygonGroup;
    };
    // 要输入给VertexInstance的顶点属性数据
    struct TriangleVertex {
        int vertexIndex;
        FVector instanceNormal;
        FVector2D instanceUV;
    };

    // 纹理编号
    std::map<FPolygonGroupID, std::string> _materialIdMap;
    // 纹理实例
    std::map<std::string, UMaterialInstanceDynamic*> _materialMap;

private:
    // 创建网格体数据
    UStaticMesh* CreateMeshDataFromFile(const FString& baseDir, const FString& file);
    // 创建纹理
    UTexture2D* CreateTexture(const FString& baseDir, const FString& file);
    // 向几何体中添加三角面信息
    void AddTriangleData(GlobalData& globalData, const TriangleVertex& v1, const TriangleVertex& v2, const TriangleVertex& v3);
};
