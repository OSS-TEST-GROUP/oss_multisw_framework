from setuptools import setup

package_name = "autoware_stub"

setup(
    name=package_name,
    version="0.1.0",
    packages=[package_name],
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="Jane.Kim",
    maintainer_email="jane.kim@obigo.com",
    description="Autoware stand-in for SiLS loop B stage 2",
    license="Apache-2.0",
    entry_points={
        "console_scripts": [
            # Same name as the package so `ros2 run autoware_stub autoware_stub` works,
            # matching how ss2_ego_bridge is invoked.
            "autoware_stub = autoware_stub.stub_node:main",
        ],
    },
)
