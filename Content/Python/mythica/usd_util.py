from pxr import UsdUtils; 

def convert_usd_to_usdz(source_file: str, dest_file: str) -> bool:
    return UsdUtils.CreateNewUsdzPackage(source_file, dest_file)
