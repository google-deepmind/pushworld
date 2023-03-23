python3 -m venv venv
source venv/bin/activate
pip3 install --require-hashes -r requirements.txt
echo $(python3 -c 'import os; print(os.getcwd())')/src > $(python3 -c 'import site; print(site.getsitepackages()[0])')/pushworld.pth
