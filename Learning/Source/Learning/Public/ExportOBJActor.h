// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <map>
#include <vector>
#include "ExportOBJActor.generated.h"

class AStaticMeshActor;
class UStaticMesh;

UCLASS()
class LEARNING_API AExportOBJActor : public AActor {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, meta = (ToolTip = "a mesh name in the scene"))
    FString meshName = "Table";

    UPROPERTY(EditAnywhere, meta = (ToolTip = "path root of the output file"))
    FString filePathRoot = "D://Default//Desktop//";

    AExportOBJActor();

protected:
    virtual void BeginPlay() override;

private:
    // 将静态网格体导出为OBJ文件
    void OutputMeshToOBJ();
    // 获取静态网格体的数据
    void AnalyseStaticMesh(AStaticMeshActor* mesh, UStaticMesh* meshData);
    // 从MeshData中获取三角形位置、法线、UV坐标等属性
    void GetTriangleDataFromMeshData(
        UStaticMesh* meshData, std::vector<FVector3f>& outPositions, std::vector<FVector3f>& outNormals, std::vector<FVector2f>& outUVs);
    // 从MeshData中获取STL材质
    void GetMTLFromMeshData(UStaticMesh* meshData, FString& mtlName, std::map<FString, FString>& mtlFiles);
    // 获取纹理信息, 并通过loadpng库导出为png文件
    FString ExportToPNGFile(TArray<UTexture*> textures);
    // 导出OBJ文件
    void ExportToOBJFile(FString mtlName, std::vector<FVector3f>& outPositions, std::vector<FVector3f>& outNormals,
        std::vector<FVector2f>& outUVs);
    // 导出MTL文件
    void ExportToMTLFile(FString& mtlName, std::map<FString, FString>& mtlFiles);
};
