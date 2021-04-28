def localized_package_path(package_path, locale):
    """Return the localized path for a given package path and locale"""
    return "locales/" + locale + "/" + package_path
