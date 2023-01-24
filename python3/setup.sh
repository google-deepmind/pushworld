python3 -m venv venv
source venv/bin/activate
pip3 install --upgrade pip
pip3 install -r requirements.txt
echo "../../../../src" > $(python3 -c 'import site; print(site.getsitepackages()[0])')/pushworld.pth
