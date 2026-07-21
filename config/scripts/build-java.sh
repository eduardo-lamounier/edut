#!/bin/bash

# Script that creates a default Java project

read -p "Enter the project name: " PROJECT_NAME

echo "Creating the project folder..."
mkdir $PROJECT_NAME
cd $PROJECT_NAME

echo "Creating a git repository..."
git init --quiet

echo "Creating src/ and bin/..."
mkdir src/ bin/

echo "Creating a .gitignore..."
touch .gitignore
echo "bin/
" >> .gitignore

echo "Creating App.java..."
touch src/App.java
echo 'public class App {
  public static void main(String[] args) {
    System.out.println("Hello world!");
  }
}
' >> src/App.java

echo "Creating a bash run script..."
touch run.sh
chmod +x run.sh
echo "javac -cp src/ src/App.java -d bin/

java -cp bin/ App

" >> run.sh

echo "Project created successfully!"
