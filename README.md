# 🌀 clisp

> **Un interpréteur Lisp compact et fonctionnel en C avec gestion des pointeurs en comptage de références.**

## 🚀 Présentation

**clisp** est un interpréteur minimaliste de **Lisp**, écrit en **C**, qui implémente une gestion mémoire en **compteur de références** pour éviter les fuites et garantir une allocation efficace. L'objectif est de fournir une base simple, extensible et performante pour l'évaluation de code Lisp.

## ✨ Fonctionnalités

- 🔹 **Évaluation d'expressions Lisp** en mode REPL
- 🔹 **Gestion mémoire automatique** via un **compteur de références**
- 🔹 **Implémentation compacte et portable** (sans dépendances externes)
- 🔹 **Primitives de base** : listes, symboles, closures, et plus encore
- 🔹 **Facilité d'extension** pour ajouter de nouvelles fonctions natives en C

## 🔧 Compilation & Exécution

### 📦 Prérequis
- Un compilateur **C (GCC, Clang, etc.)**
- **Make** (optionnel pour automatiser la compilation)

### 🛠️ Compilation
```bash
git clone https://github.com/kdridi/clisp.git
cd clisp
make
```

### 🚀 Lancer l’interpréteur
```bash
./clisp
```

### 📜 Exemples d'utilisation
```lisp
(+ 1 2)
(lambda (x) (* x x))
(define square (lambda (x) (* x x)))
(square 4)
```

## 🤝 Contribuer
Les contributions sont les bienvenues ! Ouvre une issue ou propose un pull request 🚀.

## 🧑‍💻 Auteur
Développé par kdridi avec ❤️.
⭐ N’hésite pas à starrer le projet si tu le trouves utile !
