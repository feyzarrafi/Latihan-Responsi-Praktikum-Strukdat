#include <iostream>
#include <string>
using namespace std;

//Bantu
//Mengubah ke huruf kecil supaya pencarian tidak peduli huruf besar/kecil
string lower(string s) {
    for(int i = 0; i < (int)s.length(); i++) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = s[i] + ('a' - 'A');
    }
    return s;
}

//Queue
struct QNode {
    string pelanggan;
    QNode* next;
};

struct Queue {
    QNode* head = nullptr;
    QNode* tail = nullptr;

    bool kosong() {
        return head == nullptr;
    }

    void enqueue(string p) {
        QNode* n = new QNode{p, nullptr};
        if(kosong()) {
            head = tail = n;
        }else {
            tail->next = n; tail = n;
        }
    }

    //dipakai waktu undo pembaruan menu: sambungkan queue "depan" ke bagian depan antrian ini
    //urutan pelanggan di "depan" tetap terjaga)
    void sambungDepan(Queue& depan) {
        if(depan.kosong()) {
            return;
        }
        depan.tail->next = head;
        if(tail == nullptr) {
            tail = depan.tail;
        }
        head = depan.head;
        depan.head = depan.tail = nullptr;
    }

    string dequeue() {
        if(kosong()) {
            return "";
        }
        QNode* t = head;
        string p = t->pelanggan;
        head = head->next;
        if(head == nullptr) {
            tail = nullptr;
        }
        delete t;
        return p;
    }

    //dipakai waktu undo pesanan yang masuk ke waiting list (hapus kemunculan terakhir)
    void hapusTerakhir(string p) {
        QNode *cur = head, *prev = nullptr, *targetPrev = nullptr, *target = nullptr;
        while(cur) {
            if (lower(cur->pelanggan) == lower(p)) { target = cur; targetPrev = prev; }
            prev = cur;
            cur = cur->next;
        }
        if(!target) {
            return;
        }
        if(targetPrev) {
            targetPrev->next = target->next;
        } else {
            head = target->next;
        }
        if(target == tail) {
            tail = targetPrev;
        }
        delete target;
    }

    void clear() {
        while(!kosong()) {
            dequeue();
        }
    }
};

//Data Menu
struct Menu {
    string nama, kategori;
    int harga;
    int stok;               //stok > 0 = Tersedia, stok == 0 = Habis
    Queue waiting;
    Menu* kiri = nullptr;
    Menu* kanan = nullptr;
};

Menu* cari(Menu* root, string nama) {
    string a = lower(nama);
    while(root) {
        string b = lower(root->nama);
        if(a == b) {
            return root;
        }
        root = (a < b) ? root->kiri : root->kanan;
    }
    return nullptr;
}

Menu* tambah(Menu* root, Menu* baru) {
    if(!root) {
        return baru;
    }
    if(lower(baru->nama) < lower(root->nama)) {
        root->kiri = tambah(root->kiri, baru);
    } else {
        root->kanan = tambah(root->kanan, baru);
    }
    return root;
}

Menu* minNode(Menu* n) {
    while(n->kiri) {
        n = n->kiri;
    }
    return n;
}

Menu* hapus(Menu* root, string nama) {
    if(!root) {
        return nullptr;
    }
    string a = lower(nama), b = lower(root->nama);
    if(a < b) {
        root->kiri = hapus(root->kiri, nama);
    }
    else if(a > b) {
        root->kanan = hapus(root->kanan, nama);
    } else {
        if (!root->kiri) {
            Menu* t = root->kanan;
            root->waiting.clear();
            delete root;
            return t;
        } else if(!root->kanan) {
            Menu* t = root->kiri;
            root->waiting.clear();
            delete root;
            return t;
        }
        //dua anak: ganti dengan successor (inorder), pindahkan datanya termasuk antrian
        Menu* s = minNode(root->kanan);
        root->waiting.clear();
        root->nama = s->nama;
        root->kategori = s->kategori;
        root->harga = s->harga;
        root->stok = s->stok;
        root->waiting = s->waiting;                    //salin pointer head/tail
        s->waiting.head = s->waiting.tail = nullptr;   //biar gak ikut terhapus
        root->kanan = hapus(root->kanan, s->nama);
    }
    return root;
}

void inorder(Menu* root, int& no) {
    if(!root) {
        return;
    }
    inorder(root->kiri, no);

    cout << no++ << ". " << root->nama << " | " << root->kategori << " | Rp"
         << root->harga << " | Stok: " << root->stok << " | "
         << (root->stok > 0 ? "Tersedia" : "Habis") << endl;
    inorder(root->kanan, no);
}

//STACK (riwayat aksi untuk undo)
struct Aksi {
    char jenis;            //'P' = pesanan, 'U' = pembaruan menu (tersedia kembali)
    string menu;
    string pelanggan;      //cuma buat 'P': nama pemesan
    bool masukWaiting;     //cuma buat 'P'
    Queue dilayani;        //cuma buat 'U': pelanggan waiting list yang langsung dilayani
    Aksi* next;
};

struct Stack {
    Aksi* top = nullptr;

    void push(char jenis, string menu, string pelanggan, bool waiting, Queue dilayani) {
        top = new Aksi{jenis, menu, pelanggan, waiting, dilayani, top};
    }

    bool kosong() {
        return top == nullptr;
    }

    Aksi pop() {
        Aksi* t = top;
        Aksi hasil = *t;
        top = top->next;
        delete t;
        return hasil;
    }
};

//Fitur Program
Menu* root = nullptr;
Stack riwayat;

void tambahMenu() {
    Menu* m = new Menu();
    cout << "Nama menu  : "; 
    getline(cin, m->nama);
    if(cari(root, m->nama)) {
        cout << "Menu sudah ada" << endl;
        delete m;
        return;
    }

    cout << "Kategori   : "; 
    getline(cin, m->kategori);
    cout << "Harga      : "; 
    cin >> m->harga;
    cout << "Stok       : "; 
    cin >> m->stok; 
    cin.ignore(1000, '\n');
    if(m->stok < 1) {
        cout << "Stok awal minimal 1" << endl;
        delete m;
        return;
    }
    root = tambah(root, m);
    cout << "Menu berhasil ditambahkan" << endl;
}

void lihatMenu() {
    if(!root) {
        cout << "Belum ada menu" << endl; 
        return;
    }
    int no = 1;
    inorder(root, no);
}

void pesan() {
    string pel, nama;
    cout << "Nama pelanggan : "; 
    getline(cin, pel);
    cout << "Nama menu      : "; 
    getline(cin, nama);
    Menu* m = cari(root, nama);
    if(!m) {
        cout << "Menu tidak ditemukan" << endl; return;
    }

    Queue kosong;
    if (m->stok > 0) {
        m->stok--;   //kalau stoknya 0, menu otomatis "Habis"
        cout << "Pesanan " << pel << " berhasil dibuat" << endl;
        riwayat.push('P', m->nama, pel, false, kosong);
    } else {
        cout << "Menu sedang habis, " << pel << " masuk ke dalam waiting list" << endl;
        m->waiting.enqueue(pel);
        riwayat.push('P', m->nama, pel, true, kosong);
    }
}

void tersediaKembali() {
    string nama;
    int tambahStok;

    cout << "Nama menu : "; 
    getline(cin, nama);
    Menu* m = cari(root, nama);
    if(!m) {
        cout << "Menu tidak ditemukan" << endl; return;
    }
    if(m->stok > 0) {
        cout << "Menu masih tersedia" << endl; return;
    }

    cout << "Jumlah stok baru : "; 
    cin >> tambahStok; 
    cin.ignore(1000, '\n');
    if(tambahStok < 1) { cout << "Jumlah stok minimal 1" << endl;
        return; 
    }

    m->stok = tambahStok;
    cout << "Menu tersedia kembali" << endl;

    // layani waiting list waktu stok masih ada
    Queue dilayani;
    while(m->stok > 0 && !m->waiting.kosong()) {
        string pelanggan = m->waiting.dequeue();
        m->stok--;
        dilayani.enqueue(pelanggan);
        cout << "Pesanan langsung diberikan kepada " << pelanggan << " dari waiting list" << endl;
    }
    riwayat.push('U', m->nama, "", false, dilayani);
}

void hapusMenu() {
    string nama;
    cout << "Nama menu : "; 
    getline(cin, nama);
    if(!cari(root, nama)) {
        cout << "Menu tidak ditemukan" << endl; return;
    }
    root = hapus(root, nama);
    cout << "Menu berhasil dihapus" << endl;
}

void undo() {
    if(riwayat.kosong()) {
        cout << "Tidak ada aksi untuk di-undo" << endl; return;
    }
    Aksi a = riwayat.pop();
    Menu* m = cari(root, a.menu);
    if(!m) {
        a.dilayani.clear();
        cout << "Menu " << a.menu << " sudah tidak ada, undo dilewati" << endl;
        return;
    }

    if(a.jenis == 'P') {
        if (a.masukWaiting) m->waiting.hapusTerakhir(a.pelanggan);
        else m->stok++;   //pesanan langsung dibatalkan, stok balik lagi
        cout << "Undo: Pesanan menu " << m->nama << " dibatalkan" << endl;
    } else {
        m->stok = 0;
        m->waiting.sambungDepan(a.dilayani);   //pelanggan yang tadi dilayani balik ke antrian
        cout << "Undo: Menu " << m->nama << " kembali habis" << endl;
    }
}

void tampilAntrian() {
    string nama;
    cout << "Nama menu : "; 
    getline(cin, nama);
    Menu* m = cari(root, nama);
    if(!m) {
        cout << "Menu tidak ditemukan" << endl; return;
    }
    if(m->waiting.kosong()) {
        cout << "Antrian kosong" << endl; return;
    }
    for(QNode* p = m->waiting.head; p; p = p->next){
        cout << p->pelanggan << " menunggu menu " << m->nama << endl;
    }
}

int main() {
    int pilih;
    do {
        cout << "Restoran Cipluk" << endl;
        cout << "1. Tambah menu" << endl;
        cout << "2. Lihat daftar menu" << endl;
        cout << "3. Catat pesanan" << endl;
        cout << "4. Restock Menu" << endl;
        cout << "5. Hapus menu" << endl;
        cout << "6. Undo" << endl;
        cout << "7. Tampilkan antrian" << endl;
        cout << "0. Keluar" << endl;
        cout << "Pilih: ";
        cin >> pilih; cin.ignore(1000, '\n');
        cout << endl;

        switch (pilih) {
            case 1: tambahMenu(); break;
            case 2: lihatMenu(); break;
            case 3: pesan(); break;
            case 4: tersediaKembali(); break;
            case 5: hapusMenu(); break;
            case 6: undo(); break;
            case 7: tampilAntrian(); break;
            case 0: cout << "Terima kasih!" << endl; break;
            default: cout << "Pilihan tidak valid" << endl;
        }
    } while (pilih != 0);
    return 0;
}