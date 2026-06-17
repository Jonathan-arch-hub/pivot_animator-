#!/bin/bash
# ═══════════════════════════════════════════════════════════
# Pivot Animator Pro — سكريبت التثبيت والبناء على Arch Linux
# ═══════════════════════════════════════════════════════════

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "═══════════════════════════════════════"
echo " Pivot Animator Pro — Build & Install"
echo "═══════════════════════════════════════"

# ── 1. تحقق من التبعيات ──────────────────────────────────
echo ""
echo "► التحقق من التبعيات..."

check_pkg() {
    if ! pacman -Q "$1" &>/dev/null; then
        echo "  ✗ $1 غير مثبت"
        MISSING="$MISSING $1"
    else
        echo "  ✓ $1"
    fi
}

MISSING=""
check_pkg "qt6-base"
check_pkg "cmake"
check_pkg "base-devel"

if [ -n "$MISSING" ]; then
    echo ""
    echo "► تثبيت التبعيات الناقصة..."
    sudo pacman -S --needed --noconfirm $MISSING
fi

# ── 2. بناء المشروع ──────────────────────────────────────
echo ""
echo "► بناء المشروع..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)

echo ""
echo "✓ تم البناء بنجاح!"
echo ""

# ── 3. خيار التثبيت ──────────────────────────────────────
read -p "► هل تريد تثبيته في /usr/bin؟ [y/N] " INSTALL
if [[ "$INSTALL" =~ ^[Yy]$ ]]; then
    sudo make install
    echo "✓ تم التثبيت في /usr/bin/pivot_animator"
    
    # .desktop file
    sudo tee /usr/share/applications/pivot-animator.desktop > /dev/null << 'DESKTOP'
[Desktop Entry]
Name=Pivot Animator Pro
Name[ar]=برنامج الأنيميشن
Comment=2D Stick Figure Animation Tool
Exec=pivot_animator
Icon=applications-graphics
Terminal=false
Type=Application
Categories=Graphics;Animation;
DESKTOP
    echo "✓ تم إنشاء ملف .desktop"
else
    echo ""
    echo "► للتشغيل المباشر بدون تثبيت:"
    echo "   $BUILD_DIR/pivot_animator"
fi

echo ""
echo "═══════════════════════════════════════"
echo " الاختصارات المهمة:"
echo "  S     — أداة التحديد"
echo "  N     — إضافة مفصل"
echo "  B     — رسم عظمة"
echo "  F5    — تشغيل/إيقاف"
echo "  Del   — حذف مفصل محدد"
echo "  عجلة  — تكبير/تصغير"
echo "  Alt+سحب — تحريك الكانفس"
echo "  F9    — محرر الكود"
echo "  Ctrl+E — تصدير"
echo "═══════════════════════════════════════"
