// Fill out your copyright notice in the Description page of Project Settings.

#include "ExportOBJActor.h"
#include <EngineUtils.h>
#include <Engine/StaticMeshActor.h>
#include <StaticMeshDescription.h>
#include <StaticMeshAttributes.h>
#include <iostream>
#include <fstream>
#include "./Learning/lodepng.h"

DEFINE_LOG_CATEGORY_STATIC(LogExportOBJActor, All, All);

AExportOBJActor::AExportOBJActor() {
    PrimaryActorTick.bCanEverTick = false;
}

// 将静态网格体导出为OBJ文件
void AExportOBJActor::ExportMeshToOBJ() {
    UE_LOG(LogExportOBJActor, Warning, TEXT("---! 开始导出网格体: %s !---"), *meshName);
    
    // 找到Table对应的Actor
    AStaticMeshActor* mesh = nullptr;
    for (TActorIterator<AStaticMeshActor> it(GetWorld()); it; ++it) {
        mesh = *it;
        if (mesh->GetName() == meshName) break;
    }
    if (!mesh) return;

    // 获取静态网格体数据
    UStaticMeshComponent* component = mesh->GetStaticMeshComponent();
    if (!component) {
        UE_LOG(LogExportOBJActor, Error, TEXT("网格体 %s 没有静态网格体组件"), *meshName);
        return;
    }
    meshData = component->GetStaticMesh();
    AnalyseStaticMesh();
    
    UE_LOG(LogExportOBJActor, Warning, TEXT("---! 网格体 %s 导出结束 !---"), *meshName);
}

// 获取静态网格体的数据
void AExportOBJActor::AnalyseStaticMesh() {
    if (!meshData) return;

    // 输出一些基础信息
    LogBasicData();

    // 从MeshData中获取三角形位置、法线、UV坐标、STL材质
    GetTriangleDataFromMeshData();
    GetMTLFromMeshData();

    // 导出MTL、OBJ文件
    ExportToMTLFile();
    ExportToOBJFile();
}

// 从MeshData中获取三角形位置、法线、UV坐标等属性
void AExportOBJActor::GetTriangleDataFromMeshData() {
    if (!meshData) return;
    UStaticMeshDescription* description = meshData->GetStaticMeshDescription(0);

    // 获取顶点数组，顶点实例属性数组，以及材质名称数组
    const TVertexAttributesRef<FVector3f>& positions = description->GetVertexPositions();
    const TAttributesSet<FVertexInstanceID>& attributes = description->VertexInstanceAttributes();
    TPolygonGroupAttributesConstRef<FName> materials = description->GetPolygonGroupMaterialSlotNames();

    // 遍历所有多边形组, 从中找到所有的多边形对象
    UE_LOG(LogExportOBJActor, Display, TEXT("-ATTR-[多边形组数量]: %d"), description->GetPolygonGroupCount());
    for (int i = 0; i < description->GetPolygonGroupCount(); i++) {
        TArray<FPolygonID> polygons;
        description->GetPolygonGroupPolygons(i, polygons);
        UE_LOG(LogExportOBJActor, Display, TEXT("-ATTR-[第%d个多边形组]: 多边形数量 = %d, 对应材质名称 = %s"), i, polygons.Num(), *materials[i].ToString());

        // 遍历所有的多边形对象, 从中获取所有的三角形图元(通常只有1个)
        for (FPolygonID pID : polygons) {
            TArray<FTriangleID> triangles;
            description->GetPolygonTriangles(pID, triangles);
            UE_LOG(LogExportOBJActor, Display, TEXT("-ATTR-[第%d个多边形]: 三角形数量 = %d"), pID, triangles.Num());

            // 遍历所有的三角形图元, 从中获取每个三角形对应的顶点实例索引值
            for (FTriangleID tID : triangles) {
                TArray<FVertexInstanceID> instanceIDs;
                description->GetTriangleVertexInstances(tID, instanceIDs);
                UE_LOG(LogExportOBJActor, Display, TEXT("-ATTR-[第%d个三角形]: 三个顶点实例的索引值为 %d, %d, %d"), tID, instanceIDs[0], instanceIDs[1], instanceIDs[2]);

                // 通过三个顶点的具体索引，从之前的顶点位置数组positions、顶点实例属性数组attributes中获取对应的向量数据
                for (int t = 0; t < 3; t++) {
                    FVertexInstanceID instanceID = instanceIDs[t];
                    FVertexID vID = description->GetVertexInstanceVertex(instanceID);

                    FVector3f position = positions[vID];
                    FVector3f normal = attributes.GetAttribute<FVector3f>(instanceID, MeshAttribute::VertexInstance::Normal, 0);
                    FVector2f uv = attributes.GetAttribute<FVector2f>(instanceID, MeshAttribute::VertexInstance::TextureCoordinate, 0);

                    outPositions.push_back(position);
                    outNormals.push_back(normal);
                    outUVs.push_back(uv);
                }
            }
        }
    }
}

// 从MeshData中获取STL材质
void AExportOBJActor::GetMTLFromMeshData() {
    if (!meshData) return;
    UStaticMeshDescription* description = meshData->GetStaticMeshDescription(0);
    TPolygonGroupAttributesConstRef<FName> materials = description->GetPolygonGroupMaterialSlotNames();
    
    // 遍历所有多边形组, 从中找到所有的多边形对象
    UE_LOG(LogExportOBJActor, Display, TEXT("-MTL-[多边形组数量]: %d"), description->GetPolygonGroupCount());
    for (int i = 0; i < description->GetPolygonGroupCount(); i++) {
        // 获取该多边形组对应的MTL材质
        int mtlIndex = meshData->GetMaterialIndex(materials[i]);
        UMaterialInterface* mtl = meshData->GetMaterial(mtlIndex);
        mtlName = mtl->GetName();
        UE_LOG(LogExportOBJActor, Display, TEXT("-ATTR-[第%d个多边形组]: 对应材质名称 = %s"), i, *mtlName);

        // 编辑器模式下: 获取BaseColor和Normal两个属性各自对应的工作流中包含的所有纹理对象
        // 颜色贴图
        TArray<UTexture*> textures_BaseColor;
        mtl->GetTexturesInPropertyChain(EMaterialProperty::MP_BaseColor, textures_BaseColor, NULL, NULL);
        mtlFiles["map_Ka"] = ExportToPNGFile(textures_BaseColor);
        // 法线贴图
        TArray<UTexture*> textures_Normal;
        mtl->GetTexturesInPropertyChain(EMaterialProperty::MP_Normal, textures_Normal, NULL, NULL);
        mtlFiles["bump"] = ExportToPNGFile(textures_Normal);
        
        // 运行模式下: 获取当前材质用到的所有纹理对象, 参数分别为
        // 输出的纹理对象组，材质质量级别，是否输出全部材质质量，渲染层级别，是否输出全部渲染层级别
        // mtl->GetUsedTextures(textures_BaseColor, EMaterialQualityLevel::Num, false, ERHIFeatureLevel::Num, true);
        // mtlFiles["map_Ka"] = ExportToPNGFile(textures_BaseColor);
    }
}


// 输出网格体的基础信息: LOD层级数、顶点数、三角形面数
void AExportOBJActor::LogBasicData() {
    // LOD层级数
    int numLOD = meshData->GetNumLODs();
    UE_LOG(LogExportOBJActor, Display, TEXT("%s: LOD层级数: %d"), *meshData->GetName(), numLOD);
    // 第1个LOD级别内的顶点和三角形面总数
    int numVertices = meshData->GetNumVertices(0);
    int numTriangles = meshData->GetNumTriangles(0);
    UE_LOG(LogExportOBJActor, Display, TEXT("%s: 顶点数: %d, 三角形面数: %d"), *meshData->GetName(), numVertices, numTriangles);
}

// 获取纹理信息, 并通过loadpng库导出为png文件(只考虑了一张纹理)
FString AExportOBJActor::ExportToPNGFile(TArray<UTexture*> textures) {
    // 输出的文件路径
    FString resultFileName;

    for (UTexture* texture : textures) {
        UTexture2D* texture2D = dynamic_cast<UTexture2D*>(texture);
        if (!texture2D) continue;  // 只处理2D纹理

        // 获取纹理的长、宽
        int w = texture2D->GetSizeX();
        int h = texture2D->GetSizeY();
        UE_LOG(LogExportOBJActor, Display, TEXT("-Texture-[纹理%s]的大小为: %d x %d"), *texture2D->GetFName().ToString(), w, h);

        // 获取图像的具体像素数据, 并且将它们保存到我们outImageData中
        std::vector<unsigned char> outImageData;

        // 获取只读锁可能会失败, 因此我们需要首先转换纹理对象为缺省状态
        TextureCompressionSettings prevCompression = texture2D->CompressionSettings;
        TextureMipGenSettings prevMipSettings = texture2D->MipGenSettings;
        uint8 prevSRGB = texture2D->SRGB;
        texture2D->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
        texture2D->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
        texture2D->SRGB = false;
        texture2D->UpdateResource();

        // 找到纹理对象Mipmaps第1层数据, 临时锁定当前的纹理对象
        // 获取到像素数据的内存地址, 进行读取处理之后, 再通过Unlock()结束锁定
        const FColor* imageData = static_cast<const FColor*>(texture2D->PlatformData->Mips[0].BulkData.LockReadOnly());
        if (imageData != NULL) {
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x) {
                    FColor color = imageData[y * w + x];
                    outImageData.push_back(color.R);
                    outImageData.push_back(color.G);
                    outImageData.push_back(color.B);
                    outImageData.push_back(color.A);
                }
        }
        texture2D->PlatformData->Mips[0].BulkData.Unlock();

        // 使用loadpng库导出文件
        FString filePath = filePathRoot + texture2D->GetFName().ToString();
        std::string fileName = TCHAR_TO_UTF8(*filePath);
        unsigned int result = lodepng::encode(fileName + ".png", outImageData, w, h);
        if (result > 0) {
            UE_LOG(LogExportOBJActor, Error, TEXT("-Texture-[lodepng]导出失败, 提示信息为: %s"), UTF8_TO_TCHAR(lodepng_error_text(result)));
        } else {
            UE_LOG(LogExportOBJActor, Warning, TEXT("--! 导出MTL文件成功, 路径为 %s !--"), *filePath);
        }
        resultFileName = UTF8_TO_TCHAR((fileName + ".png").c_str());

        // 将纹理参数设置回原状
        texture2D->CompressionSettings = prevCompression;
        texture2D->MipGenSettings = prevMipSettings;
        texture2D->SRGB = prevSRGB;
        texture2D->UpdateResource();
    }

    return resultFileName;
}

// 导出MTL文件
void AExportOBJActor::ExportToMTLFile() {
    FString filePath = filePathRoot + meshName + ".mtl";
    std::ofstream out(*filePath);

    // 材质信息
    out << "# Exported from UE5: " << TCHAR_TO_UTF8(*meshName) << std::endl;
    out << "newmtl " << TCHAR_TO_UTF8(*mtlName) << std::endl;
    
    // out << "Ka 0.2 0.2 0.2" << std::endl;
    // out << "Kd 0.6 0.6 0.6" << std::endl;
    // out << "Ks 0.9 0.9 0.9" << std::endl;
    
    // 纹理贴图
    for (std::map<FString, FString>::iterator it = mtlFiles.begin(); it != mtlFiles.end(); ++it) {
        out << TCHAR_TO_UTF8(*it->first) << " " << TCHAR_TO_UTF8(*it->second) << std::endl;
    }

    UE_LOG(LogExportOBJActor, Warning, TEXT("--! 导出MTL文件成功, 路径为 %s !--"), *filePath);
}

// 导出OBJ文件
void AExportOBJActor::ExportToOBJFile() {
    FString filePath = filePathRoot + meshName + ".obj";
    std::ofstream out(*filePath);
    
    // 注释
    out << "# Export from UE5: " << TCHAR_TO_UTF8(*meshName) << std::endl;
    out << std::endl;

    // 材质信息
    out << "mtllib " << TCHAR_TO_UTF8(*meshName) << ".mtl" << std::endl;
    out << "g " << TCHAR_TO_UTF8(*mtlName) << std::endl;
    out << std::endl;
    
    // 顶点位置
    for (size_t i = 0; i < outPositions.size(); ++i) {
        const FVector3f& v = outPositions[i];
        out << "v " 
            << v[0] << " " << v[1] << " " << v[2] << std::endl;
    }
    out << std::endl;
    
    // 顶点法线
    for (size_t i = 0; i < outNormals.size(); ++i) {
        const FVector3f& n = outNormals[i];
        out << "vn " 
            << n[0] << " " << n[1] << " " << n[2] << std::endl;
    }
    out << std::endl;
   
    // 顶点UV坐标
    for (size_t i = 0; i < outUVs.size(); ++i) {
        const FVector2f& uv = outUVs[i];
        out << "vt " 
            << uv[0] << " " << 1.0 - uv[1] << std::endl;
    }
    out << std::endl;
    
    // 三角面索引
    out << "usemtl " << TCHAR_TO_UTF8(*mtlName) << std::endl << std::endl;
    for (size_t i = 0; i < outPositions.size(); i += 3) {
        out << "f " 
            << (i + 1) << "/" << (i + 1) << "/" << (i + 1) << " " 
            << (i + 2) << "/" << (i + 2) << "/" << (i + 2) << " " 
            << (i + 3) << "/" << (i + 3) << "/" << (i + 3) << std::endl;
    }

    UE_LOG(LogExportOBJActor, Warning, TEXT("--! 导出OBJ文件成功, 路径为 %s !--"), *filePath);
}


