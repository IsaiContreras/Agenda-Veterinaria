#include <Windows.h>
#include <fstream>
#include <string>
#include <time.h>
#include "resource.h"
using namespace std;

//msdn
//StackOverflow
//C SHEEL
//Videojuego TIS 100

#define TM_RELOJ 3000
#define TM_NC_RELOJ 3001
#define TM_EDC_RELOJ 3002

#pragma region VariablesGlobales
HINSTANCE hInstGlobal;
HMENU hBarraMenu;
HWND hAgenda;
HWND hNuevaCita;
HWND hEditarCita;
HWND hPagarCita;
HWND hEditarDoctor;

HWND hLbAgenda;
HWND hLblReloj;
HWND hLblNombreMedico;
HWND hLblCedula;
HWND hPcFotoDoctor;
HWND hPcFotoMascota;

HBITMAP hBmpDoctor;
HBITMAP hBmpMascota;
char tempImg1[MAX_PATH];
char tempImg2[MAX_PATH];

time_t allTime;
struct tm *tiempoActual;

struct CITA {
	CITA *prev;
	char nombreDueño[50];
	char nombreMascota[50];
	string telefono;
	string especie;
	char motivoConsulta[MAX_PATH];
	char image1[MAX_PATH] = "";
	char image2[MAX_PATH] = "";
	string fechaString;
	string horaString;
	int year;
	int month;
	int day;
	int hour;
	int minutes;
	int especieIndex;
	int formaPago;
	float costo;
	CITA *next;
}*origin, *aux;
struct DOCTOR {
	char nombreMedico[50] = { NULL };
	char cedula[30] = { NULL };
	char chDirFotoDoc[MAX_PATH] = "";
}*doc;

fstream archivo;

char chCambioFoto[MAX_PATH] = "";
bool salida = false;
int indexImage;
#pragma endregion

#pragma region PrototipoFunciones
//Archivos
void saveDoctor();
void saveLista(CITA *origin);
void loadDoctor();
void loadLista();
//Operaciones
bool verificarNum(string);
bool verificarAlfa(string);
bool introducirDatos(HWND);
void ordenamiento();
void intercambio(CITA *auxActual, CITA *auxProx);
void impresion();
void limpiarMostrador(HWND);
void fotoDoctor(HWND);
void borrarFotoMascota(HWND);
void borrarFotoDoctor(HWND);
#pragma endregion

#pragma region PrototipoFunciones Ventana
BOOL CALLBACK agendaVentanaPrincipal(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK nuevaCita(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK editarCita(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK pagarCita(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK editarInfoDoctor(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK primerDoctor(HWND, UINT, WPARAM, LPARAM);
#pragma endregion

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, PSTR cmdLine, int cShow) {
	origin = aux = NULL;
	hInstGlobal = hInst;

	loadDoctor();
	loadLista();

	hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
	ShowWindow(hAgenda, SW_SHOW);
	SetTimer(hAgenda, TM_RELOJ, 1000, NULL);

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	saveDoctor();
	saveLista(origin);
	return 0;
}

#pragma region FuncionesVentanas
BOOL CALLBACK agendaVentanaPrincipal(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG: {
		hBarraMenu = GetMenu(hwnd);
		EnableMenuItem(hBarraMenu, BTN_AGENDA, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_NUEVACITA, MF_ENABLED);
		EnableMenuItem(hBarraMenu, BTN_EDITDOCINFO, MF_ENABLED);
		EnableMenuItem(hBarraMenu, BTN_SALIR, MF_ENABLED);

		SendMessage(hLbAgenda, LB_RESETCONTENT, 0, 0);

		ordenamiento();

		hLbAgenda = GetDlgItem(hwnd, IDC_LISTACITAS);
		impresion();

		hLblReloj = GetDlgItem(hwnd, ST_MENU_RELOJ);

		hLblNombreMedico = GetDlgItem(hwnd, ST_MENU_DOCTOR);
		hLblCedula = GetDlgItem(hwnd, ST_MENU_CEDULA);

		if (doc != NULL) {
			SetWindowText(hLblNombreMedico, doc->nombreMedico);
			SetWindowText(hLblCedula, doc->cedula);
		}
		else {
			int primerDoc = DialogBox(hInstGlobal, MAKEINTRESOURCE(IDD_PRIMERDOCTOR), hwnd, primerDoctor);
		}
		hPcFotoDoctor = GetDlgItem(hwnd, BMP_MENU_DOCTOR);
		hBmpDoctor = (HBITMAP)LoadImage(NULL, doc->chDirFotoDoc, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
		SendMessage(hPcFotoDoctor, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpDoctor);
		
		HWND hLblListCount = GetDlgItem(hwnd, ST_LISTCOUNT);
		int lista = SendMessage(hLbAgenda, LB_GETCOUNT, 0, 0);
		string n = to_string(lista);
		char listaC[20];
		strcpy(listaC, n.c_str());
		SetWindowText(hLblListCount, listaC);
	}break;
	case WM_COMMAND: {
		//OPCIONES DE BARRA DE MENU
		if (LOWORD(wParam) == BTN_NUEVACITA && HIWORD(wParam) == BN_CLICKED) {
			aux = origin;
			KillTimer(hAgenda, TM_RELOJ);
			DestroyWindow(hAgenda);
			hNuevaCita = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_NUEVACITA), NULL, nuevaCita);
			ShowWindow(hNuevaCita, SW_SHOW);
			SetTimer(hNuevaCita, TM_NC_RELOJ, 1000, NULL);
		}
		if (LOWORD(wParam) == BTN_EDITDOCINFO && HIWORD(wParam) == BN_CLICKED) {
			KillTimer(hAgenda, TM_RELOJ);
			DestroyWindow(hAgenda);
			hEditarDoctor = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_EDITDOCTOR), NULL, editarInfoDoctor);
			ShowWindow(hEditarDoctor, SW_SHOW);
		}
		if (LOWORD(wParam) == BTN_SALIR && HIWORD(wParam) == BN_CLICKED) {
			if (MessageBox(hwnd, "¿Seguro que quiere salir del programa?", "SALIR", MB_YESNO) == IDYES) {
				salida = true;
				KillTimer(hwnd, TM_RELOJ);
				DestroyWindow(hAgenda);
			}
			break;
		}
		//GESTIÓN DE CITAS
		if (LOWORD(wParam) == BTN_SELECT && HIWORD(wParam) == BN_CLICKED) {
			int index = SendMessage(hLbAgenda, LB_GETCURSEL, 0, 0);
			if (index > -1) {
				char buffer[80];
				SendMessage(hLbAgenda, LB_GETTEXT, index, (LPARAM)buffer);
				string cita(buffer);

				int n = cita.size();
				int toFirstNumStr = -1;
				for (int i = 0; i < n; i++) {
					toFirstNumStr++;
					if (cita[i] >= 48 && cita[i] <= 57) {
						break;
					}
				}

				string fechaLista = cita.substr(toFirstNumStr, 10);
				string horaLista = cita.substr(toFirstNumStr + 15, 14);

				aux = origin;
				while (aux != NULL) {
					if (fechaLista.compare(aux->fechaString) == 0) {
						if (horaLista.compare(aux->horaString) == 0) {
							break;
						}
					}
					aux = aux->next;
				}

				strcpy(chCambioFoto, aux->image1);
				hPcFotoMascota = GetDlgItem(hwnd, BMP_MENU_MASCOTA1);
				hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 90, 108, LR_LOADFROMFILE);
				SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);

				strcpy(chCambioFoto, aux->image2);
				hPcFotoMascota = GetDlgItem(hwnd, BMP_MENU_MASCOTA2);
				hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 90, 108, LR_LOADFROMFILE);
				SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);

				HWND hStMascota = GetDlgItem(hwnd, ST_INFO_MASCOTA);
				SetWindowText(hStMascota, aux->nombreMascota);

				HWND hStEspecie = GetDlgItem(hwnd, ST_INFO_ESPECIE);
				strcpy(buffer, aux->especie.c_str());
				SetWindowText(hStEspecie, buffer);

				HWND hStDueno = GetDlgItem(hwnd, ST_INFO_DUENO);
				SetWindowText(hStDueno, aux->nombreDueño);

				HWND hStTelefono = GetDlgItem(hwnd, ST_INFO_TELEFONO);
				strcpy(buffer, aux->telefono.c_str());
				SetWindowText(hStTelefono, buffer);

				HWND hStFechaHora = GetDlgItem(hwnd, ST_INFO_FECHA);
				string infoFechaHora = aux->fechaString + " a las " + aux->horaString;
				strcpy(buffer, infoFechaHora.c_str());
				SetWindowText(hStFechaHora, buffer);

				HWND hStCosto = GetDlgItem(hwnd, ST_INFO_COSTO);
				char costoC[20];
				snprintf(costoC, sizeof(costoC), "%.2f", aux->costo);
				string costo(costoC);
				if (aux->formaPago == 3) {
					costo = "$" + costo + " a 3 meses sin intereses.";
				}
				else if (aux->formaPago == 6) {
					costo = "$" + costo + " a 6 meses sin intereses.";
				}
				else if (aux->formaPago == 9) {
					costo = "$" + costo + " a 9 meses sin intereses.";
				}
				else {
					costo = "$" + costo + " de contado.";
				}
				strcpy(buffer, costo.c_str());
				SetWindowText(hStCosto, buffer);

				HWND hStMotivo = GetDlgItem(hwnd, ST_INFO_MOTIVO);
				SetWindowText(hStMotivo, aux->motivoConsulta);

				HWND hBtnEdit = GetDlgItem(hwnd, BTN_EDITARCITA);
				HWND hBtnPagar = GetDlgItem(hwnd, BTN_PAGARCITA);
				HWND hBtnEliminar = GetDlgItem(hwnd, BTN_ELIMINARCITA);
				EnableWindow(hBtnEdit, true);
				EnableWindow(hBtnPagar, true);
				EnableWindow(hBtnEliminar, true);
			}
		}
		if (LOWORD(wParam) == BTN_EDITARCITA && HIWORD(wParam) == BN_CLICKED) {
			DestroyWindow(hAgenda);
			hEditarCita = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_EDITARCITA), NULL, editarCita);
			ShowWindow(hEditarCita, SW_SHOW);
			SetTimer(hEditarCita, TM_EDC_RELOJ, 1000, NULL);
		}
		if (LOWORD(wParam) == BTN_PAGARCITA && HIWORD(wParam) == BN_CLICKED) {
			DestroyWindow(hAgenda);
			hPagarCita = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_PAGOCITA), NULL, pagarCita);
			ShowWindow(hPagarCita, SW_SHOW);
		}
		if (LOWORD(wParam) == BTN_FILTER && HIWORD(wParam) == BN_CLICKED) {
			int length;
			char buff[30];
			SendMessage(hLbAgenda, LB_RESETCONTENT, 0, 0);

			HWND hTpFiltro = GetDlgItem(hwnd, IDC_DATETIMEPICKER1);
			length = GetWindowTextLength(hTpFiltro);
			GetWindowText(hTpFiltro, buff, length + 4);
			string filter(buff);

			ordenamiento();
			aux = origin;
			while (aux != NULL) {
				if (aux->fechaString.compare(filter) == 0) {
					string mascota(aux->nombreMascota);
					string display = mascota + "  |  " + aux->fechaString + "  |  " + aux->horaString;
					char buffL[100];
					strcpy(buffL, display.c_str());
					SendMessage(hLbAgenda, LB_ADDSTRING, 0, (LPARAM)buffL);
				}

				aux = aux->next;
			}
			aux = origin;
		}
		if (LOWORD(wParam) == BTN_RESTAURARLISTA && HIWORD(wParam) == BN_CLICKED) {
			SendMessage(hLbAgenda, LB_RESETCONTENT, 0, 0);
			ordenamiento();
			impresion();
		}
		if (LOWORD(wParam) == BTN_ELIMINARCITA && HIWORD(wParam) == BN_CLICKED) {
			if (MessageBox(hwnd, "¿Seguro que desea eliminar esta cita?", "Eliminar Cita", MB_OKCANCEL) == IDOK) {
					SendMessage(hLbAgenda, LB_RESETCONTENT, 0, 0);
					if (aux->next == NULL && aux->prev == NULL) {
						delete aux;
						aux = origin = NULL;
					}
					else if (aux->prev == NULL) {
						origin = origin->next;
						aux->next->prev = NULL;
						delete aux;
						aux = origin;
					}
					else if (aux->next == NULL) {
						aux->prev->next = NULL;
						delete aux;
						aux = origin;
					}
					else {
						aux->prev->next = aux->next;
						aux->next->prev = aux->prev;
						delete aux;
						aux = origin;
					}

					ordenamiento();
					impresion();

					limpiarMostrador(hwnd);
				}
		}
		if (LOWORD(wParam) == BTN_BORRARLISTA && HIWORD(wParam) == BN_CLICKED) {
			if (origin != NULL) {
				if (MessageBox(hwnd, "¿Seguro que desea borrar toda la lista? Los datos se perderán completamente.", "BORRAR LISTA", MB_YESNO) == IDYES) {
					aux = origin;
					while (aux != NULL) {
						aux = aux->next;
						delete origin;
						origin = aux;
					}
					SendMessage(hLbAgenda, LB_RESETCONTENT, 0, 0);
					limpiarMostrador(hwnd);
				}
			}
		}
	}break;
	case WM_TIMER: {
			time(&allTime);
			tiempoActual = localtime(&allTime);
			char reloj[80];
			strftime(reloj, 80, "Hoy es: %d/%m/%Y  %I:%M:%S", tiempoActual);
			SetWindowText(hLblReloj, reloj);
		}break;
	case WM_CLOSE:
		break;
	case WM_DESTROY:
		if (salida) {
			PostQuitMessage(0);
		}
	    break;
	}
	return FALSE;
}

BOOL CALLBACK nuevaCita(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG: {
		if (origin == NULL) {
			origin = new CITA;
			aux = origin;
			aux->next = NULL;
			aux->prev = NULL;
		}
		else {
			while (aux->next != NULL) {
				aux = aux->next;
			}
			aux->next = new CITA;
			aux->next->prev = aux;
			aux = aux->next;
			aux->next = NULL;
		}

		hBarraMenu = GetMenu(hwnd);
		EnableMenuItem(hBarraMenu, BTN_NUEVACITA, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_AGENDA, MF_ENABLED);
		EnableMenuItem(hBarraMenu, BTN_EDITDOCINFO, MF_ENABLED);
		EnableMenuItem(hBarraMenu, BTN_SALIR, MF_ENABLED);

		HWND hCbEspecie = GetDlgItem(hwnd, CB_NC_ESPECIE);
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Perro");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Gato");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Conejo");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Hámster");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Pájaro");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Loro");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Pez");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Tortuga");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Reptil o anfibio");

		HWND hLblNombreMedicoNC = GetDlgItem(hwnd, ST_NC_DOCTOR);
		HWND hLblCedulaNC = GetDlgItem(hwnd, ST_NC_CEDULA);
		SetWindowText(hLblNombreMedicoNC, doc->nombreMedico);
		SetWindowText(hLblCedulaNC, doc->cedula);
		hPcFotoDoctor = GetDlgItem(hwnd, BMP_NC_DOCTOR);
		hBmpDoctor = (HBITMAP)LoadImage(NULL, doc->chDirFotoDoc, IMAGE_BITMAP, 80, 96, LR_LOADFROMFILE);
		SendMessage(hPcFotoDoctor, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpDoctor);

		hLblReloj = GetDlgItem(hwnd, ST_NC_RELOJ);
		indexImage = 0;
	}break;
	case WM_COMMAND: {
		//OPCIONES BARRA DE MENU
		if (LOWORD(wParam) == BTN_AGENDA && HIWORD(wParam) == BN_CLICKED) {
			if (aux->next == NULL && aux->prev == NULL) {
				delete aux;
				aux = origin = NULL;
			}
			else {
				aux->prev->next = NULL;
				delete aux;
				aux = origin;
			}

			KillTimer(hNuevaCita, TM_NC_RELOJ);
			DestroyWindow(hNuevaCita);
			hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
			ShowWindow(hAgenda, SW_SHOW);
			SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
		}
		if (LOWORD(wParam) == BTN_EDITDOCINFO && HIWORD(wParam) == BN_CLICKED) {
			if (aux->next == NULL && aux->prev == NULL) {
				delete aux;
				aux = origin = NULL;
			}
			else {
				aux->prev->next = NULL;
				delete aux;
				aux = origin;
			}

			KillTimer(hNuevaCita, TM_NC_RELOJ);
			DestroyWindow(hNuevaCita);
			hEditarDoctor = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_EDITDOCTOR), NULL, editarInfoDoctor);
			ShowWindow(hEditarDoctor, SW_SHOW);
		}
		if (LOWORD(wParam) == BTN_SALIR && HIWORD(wParam) == BN_CLICKED) {
			if (MessageBox(hwnd, "¿Seguro que quiere salir del programa?", "SALIR", MB_YESNO) == IDYES) {
				if (aux->next == NULL && aux->prev == NULL) {
					delete aux;
					aux = origin = NULL;
				}
				else {
					aux->prev->next = NULL;
					delete aux;
					aux = origin;
				}

				salida = true;
				KillTimer(hwnd, TM_NC_RELOJ);
				DestroyWindow(hNuevaCita);
			}
		}
		//OPCIONES CITA NUEVA
		if (LOWORD(wParam) == BTN_NC_NEXT && HIWORD(wParam) == BN_CLICKED) {
			indexImage = 1;
			HWND hBtnIndexImage = GetDlgItem(hwnd, BTN_NC_NEXT);
			EnableWindow(hBtnIndexImage, false);
			hBtnIndexImage = GetDlgItem(hwnd, BTN_NC_PREV);
			EnableWindow(hBtnIndexImage, true);

			strcpy(chCambioFoto, aux->image2);
			hPcFotoMascota = GetDlgItem(hwnd, BMP_NC_MASCOTA);
			hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
			SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
			break;
		}
		if (LOWORD(wParam) == BTN_NC_PREV && HIWORD(wParam) == BN_CLICKED) {
			indexImage = 0;
			HWND hBtnIndexImage = GetDlgItem(hwnd, BTN_NC_PREV);
			EnableWindow(hBtnIndexImage, false);
			hBtnIndexImage = GetDlgItem(hwnd, BTN_NC_NEXT);
			EnableWindow(hBtnIndexImage, true);

			strcpy(chCambioFoto, aux->image1);
			hPcFotoMascota = GetDlgItem(hwnd, BMP_NC_MASCOTA);
			hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
			SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
			break;
		}
		if (LOWORD(wParam) == BTN_NC_FOTOMASCOTA && HIWORD(wParam) == BN_CLICKED) {
			OPENFILENAME ofnFotoMascota;
			ZeroMemory(&ofnFotoMascota, sizeof(ofnFotoMascota));

			ofnFotoMascota.hwndOwner = hwnd;
			ofnFotoMascota.lStructSize = sizeof(ofnFotoMascota);
			ofnFotoMascota.lpstrFile = chCambioFoto;
			ofnFotoMascota.nMaxFile = MAX_PATH;
			ofnFotoMascota.lpstrDefExt = "bmp";
			ofnFotoMascota.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
			ofnFotoMascota.lpstrFilter = "BMP Images\0*.bmp\0All Files\0*.*\0";
			if (GetOpenFileName(&ofnFotoMascota)) {
				hPcFotoMascota = GetDlgItem(hwnd, BMP_NC_MASCOTA);
				hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
				SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
			}

			if (indexImage == 0) {
				strcpy(aux->image1, chCambioFoto);
			}
			if (indexImage == 1) {
				strcpy(aux->image2, chCambioFoto);
			}
			
			break;
		}
		if (LOWORD(wParam) == BTN_NC_BORRARFOTO && HIWORD(wParam) == BN_CLICKED) {
			borrarFotoMascota(hwnd);
		}
		if (LOWORD(wParam) == BTN_NC_CANCEL && HIWORD(wParam) == BN_CLICKED) {
			if (aux->next == NULL && aux->prev == NULL) {
				delete aux;
				aux = origin = NULL;
			}
			else {
				aux->prev->next = NULL;
				delete aux;
				aux = origin;
			}

			KillTimer(hNuevaCita, TM_NC_RELOJ);
			DestroyWindow(hNuevaCita);
			hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
			ShowWindow(hAgenda, SW_SHOW);
			SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
		}
		if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED) {
			if (introducirDatos(hwnd)) {
				MessageBox(hwnd, "Cita agregada.", "DATOS CORRECTOS", MB_OK);

				aux = origin;

				KillTimer(hNuevaCita, TM_NC_RELOJ);
				DestroyWindow(hNuevaCita);
				hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
				ShowWindow(hAgenda, SW_SHOW);
				SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
			}
		}
	}break;
	case WM_TIMER: {
		time(&allTime);
		tiempoActual = localtime(&allTime);
		char reloj[80];
		strftime(reloj, 80, "Hoy es: %d/%m/%Y  %I:%M:%S", tiempoActual);
		SetWindowText(hLblReloj, reloj);
	}break;
	case WM_DESTROY:
		if (salida) {
			PostQuitMessage(0);
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK editarCita(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG: {
		hBarraMenu = GetMenu(hwnd);
		EnableMenuItem(hBarraMenu, BTN_NUEVACITA, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_AGENDA, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_EDITDOCINFO, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_SALIR, MF_DISABLED);

		HWND hCbEspecie = GetDlgItem(hwnd, CB_NC_ESPECIE);
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Perro");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Gato");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Conejo");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Hámster");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Pájaro");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Loro");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Pez");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Tortuga");
		SendMessage(hCbEspecie, CB_ADDSTRING, 0, (LPARAM)"Reptil o anfibio");

		HWND hLblNombreMedicoNC = GetDlgItem(hwnd, ST_NC_DOCTOR);
		HWND hLblCedulaNC = GetDlgItem(hwnd, ST_NC_CEDULA);
		SetWindowText(hLblNombreMedicoNC, doc->nombreMedico);
		hPcFotoDoctor = GetDlgItem(hwnd, BMP_NC_DOCTOR);
		hBmpDoctor = (HBITMAP)LoadImage(NULL, doc->chDirFotoDoc, IMAGE_BITMAP, 80, 96, LR_LOADFROMFILE);
		SendMessage(hPcFotoDoctor, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpDoctor);
		SetWindowText(hLblCedulaNC, doc->cedula);

		hLblReloj = GetDlgItem(hwnd, ST_NC_RELOJ);

		strcpy(chCambioFoto, aux->image1);
		hPcFotoMascota = GetDlgItem(hwnd, BMP_NC_MASCOTA);
		hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
		SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
		indexImage = 0;
		strcpy(tempImg1, aux->image1);
		strcpy(tempImg2, aux->image2);

		char buffer[80];
		HWND hEdNombreD = GetDlgItem(hwnd, EDT_NC_DNOMBRE);
		SetWindowText(hEdNombreD, aux->nombreDueño);

		HWND hEdTelefono = GetDlgItem(hwnd, EDT_NC_TELEFONO);
		strcpy(buffer, aux->telefono.c_str());
		SetWindowText(hEdTelefono, buffer);

		HWND hEdNombreM = GetDlgItem(hwnd, EDT_NC_MNOMBRE);
		SetWindowText(hEdNombreM, aux->nombreMascota);

		SendMessage(hCbEspecie, CB_SETCURSEL, aux->especieIndex, 0);

		HWND hStFechaA = GetDlgItem(hwnd, ST_NC_FECHAACTUAL);
		string fechaA = "Fecha de la cita: " + aux->fechaString + " a las " + aux->horaString;
		strcpy(buffer, fechaA.c_str());
		SetWindowText(hStFechaA, buffer);

		HWND hEdMotivo = GetDlgItem(hwnd, EDT_NC_MOTIVO);
		SetWindowText(hEdMotivo, aux->motivoConsulta);

		HWND hEdCosto = GetDlgItem(hwnd, EDT_NC_COSTO);
		snprintf(buffer, sizeof(buffer), "%.2f", aux->costo);
		SetWindowText(hEdCosto, buffer);

		if (aux->formaPago == 3) {
			HWND hRdMeses = GetDlgItem(hwnd, RD_NC_3MESES);
			SendMessage(hRdMeses, BM_SETCHECK, BST_CHECKED, 1);
		}
		else if (aux->formaPago == 6) {
			HWND hRdMeses = GetDlgItem(hwnd, RD_NC_6MESES);
			SendMessage(hRdMeses, BM_SETCHECK, BST_CHECKED, 1);
		}
		else if (aux->formaPago == 9) {
			HWND hRdMeses = GetDlgItem(hwnd, RD_NC_9MESES);
			SendMessage(hRdMeses, BM_SETCHECK, BST_CHECKED, 1);
		}
		else {
			HWND hRdMeses = GetDlgItem(hwnd, RD_NC_CONTADO);
			SendMessage(hRdMeses, BM_SETCHECK, BST_CHECKED, 1);
		}
	}break;
	case WM_COMMAND:
		//OPCIONES DE EDICIÓN DE CITA
		if (LOWORD(wParam) == BTN_NC_NEXT && HIWORD(wParam) == BN_CLICKED) {
			indexImage = 1;
			HWND hBtnIndexImage = GetDlgItem(hwnd, BTN_NC_NEXT);
			EnableWindow(hBtnIndexImage, false);
			hBtnIndexImage = GetDlgItem(hwnd, BTN_NC_PREV);
			EnableWindow(hBtnIndexImage, true);

			strcpy(chCambioFoto, aux->image2);
			hPcFotoMascota = GetDlgItem(hwnd, BMP_NC_MASCOTA);
			hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
			SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
			break;
		}
		if (LOWORD(wParam) == BTN_NC_PREV && HIWORD(wParam) == BN_CLICKED) {
			indexImage = 0;
			HWND hBtnIndexImage = GetDlgItem(hwnd, BTN_NC_PREV);
			EnableWindow(hBtnIndexImage, false);
			hBtnIndexImage = GetDlgItem(hwnd, BTN_NC_NEXT);
			EnableWindow(hBtnIndexImage, true);

			strcpy(chCambioFoto, aux->image1);
			hPcFotoMascota = GetDlgItem(hwnd, BMP_NC_MASCOTA);
			hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
			SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
			break;
		}
		if (LOWORD(wParam) == BTN_NC_FOTOMASCOTA && HIWORD(wParam) == BN_CLICKED) {
			OPENFILENAME ofnFotoMascota;
			ZeroMemory(&ofnFotoMascota, sizeof(ofnFotoMascota));

			ofnFotoMascota.hwndOwner = hwnd;
			ofnFotoMascota.lStructSize = sizeof(ofnFotoMascota);
			ofnFotoMascota.lpstrFile = chCambioFoto;
			ofnFotoMascota.nMaxFile = MAX_PATH;
			ofnFotoMascota.lpstrDefExt = "bmp";
			ofnFotoMascota.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofnFotoMascota.lpstrFilter = "BMP Images\0*.bmp\0All Files\0*.*\0";
			if (GetOpenFileName(&ofnFotoMascota)) {
				hPcFotoMascota = GetDlgItem(hwnd, BMP_NC_MASCOTA);
				hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
				SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
			}

			if (indexImage == 0) {
				strcpy(aux->image1, chCambioFoto);
			}
			if (indexImage == 1) {
				strcpy(aux->image2, chCambioFoto);
			}
			break;
		}
		if (LOWORD(wParam) == BTN_NC_BORRARFOTO && HIWORD(wParam) == BN_CLICKED) {
			borrarFotoMascota(hwnd);
		}
		if (LOWORD(wParam) == BTN_NC_CANCEL && HIWORD(wParam) == BN_CLICKED) {
			strcpy(aux->image1, tempImg1);
			strcpy(aux->image2, tempImg2);

			aux = origin;
			KillTimer(hwnd, TM_EDC_RELOJ);
			DestroyWindow(hEditarCita);
			hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
			ShowWindow(hAgenda, SW_SHOW);
			SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
		}
		if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED) {
			if (introducirDatos(hwnd)) {
				MessageBox(hwnd, "Cita Modificada.", "DATOS CORRECTOS", MB_OK);

				aux = origin;

				KillTimer(hEditarCita, TM_EDC_RELOJ);
				DestroyWindow(hEditarCita);
				hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
				ShowWindow(hAgenda, SW_SHOW);
				SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
			}
		}
		break;
	case WM_TIMER: {
		time(&allTime);
		tiempoActual = localtime(&allTime);
		char reloj[80];
		strftime(reloj, 80, "Hoy es: %d/%m/%Y  %I:%M:%S", tiempoActual);
		SetWindowText(hLblReloj, reloj);
	}break;
	}
	return FALSE;
}

BOOL CALLBACK pagarCita(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG: {
		hBarraMenu = GetMenu(hwnd);
		EnableMenuItem(hBarraMenu, BTN_AGENDA, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_NUEVACITA, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_EDITDOCINFO, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_SALIR, MF_DISABLED);

		HWND hLblNombreMedicoPC = GetDlgItem(hwnd, ST_PC_DOCTOR);
		HWND hLblCedulaPC = GetDlgItem(hwnd, ST_PC_CEDULA);
		SetWindowText(hLblNombreMedicoPC, doc->nombreMedico);
		SetWindowText(hLblCedulaPC, doc->cedula);
		hPcFotoDoctor = GetDlgItem(hwnd, BMP_PC_DOCTOR);
		hBmpDoctor = (HBITMAP)LoadImage(NULL, doc->chDirFotoDoc, IMAGE_BITMAP, 80, 96, LR_LOADFROMFILE);
		SendMessage(hPcFotoDoctor, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpDoctor);

		strcpy(chCambioFoto, aux->image1);
		hPcFotoMascota = GetDlgItem(hwnd, BMP_PC_MASCOTA1);
		hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 95, 114, LR_LOADFROMFILE);
		SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);

		strcpy(chCambioFoto, aux->image2);
		hPcFotoMascota = GetDlgItem(hwnd, BMP_PC_MASCOTA2);
		hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 95, 114, LR_LOADFROMFILE);
		SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);

		char buffer[80];
		HWND hStMascota = GetDlgItem(hwnd, ST_PC_MNOMBRE);
		SetWindowText(hStMascota, aux->nombreMascota);

		HWND hStEspecie = GetDlgItem(hwnd, ST_PC_ESPECIE);
		strcpy(buffer, aux->especie.c_str());
		SetWindowText(hStEspecie, buffer);

		HWND hStDueno = GetDlgItem(hwnd, ST_PC_DNOMBRE);
		SetWindowText(hStDueno, aux->nombreDueño);

		HWND hStTelefono = GetDlgItem(hwnd, ST_PC_TELEFONO);
		strcpy(buffer, aux->telefono.c_str());
		SetWindowText(hStTelefono, buffer);

		HWND hStFechaHora = GetDlgItem(hwnd, ST_PC_FECHAHORA);
		string infoFechaHora = aux->fechaString + " a las " + aux->horaString;
		strcpy(buffer, infoFechaHora.c_str());
		SetWindowText(hStFechaHora, buffer);

		HWND hStMotivo = GetDlgItem(hwnd, EDT_PC_MOTIVO);
		SetWindowText(hStMotivo, aux->motivoConsulta);

		HWND hStCosto = GetDlgItem(hwnd, ST_PC_COSTO);
		char costoC[20];
		snprintf(costoC, sizeof(costoC), "%.2f", aux->costo);
		string costo(costoC);
		if (aux->formaPago == 3) {
			costo = costo + " a 3 meses sin intereses.";
		}
		else if (aux->formaPago == 6) {
			costo = costo + " a 6 meses sin intereses.";
		}
		else if (aux->formaPago == 9) {
			costo = costo + " a 9 meses sin intereses.";
		}
		else {
			costo = costo + " de contado.";
		}
		strcpy(buffer, costo.c_str());
		SetWindowText(hStCosto, buffer);

		float iva = aux->costo * .12;
		float precioTot = aux->costo + iva;
		float precioMeses = precioTot / aux->formaPago;

		HWND hStIva = GetDlgItem(hwnd, ST_IVA);
		char ivaC[20];
		snprintf(ivaC, sizeof(ivaC), "%.2f", iva);
		SetWindowText(hStIva, ivaC);

		HWND hStPrecioTot = GetDlgItem(hwnd, ST_PRECIOTOT);
		char precioTotC[20];
		snprintf(precioTotC, sizeof(precioTotC), "%.2f", precioTot);
		SetWindowText(hStPrecioTot, precioTotC);

		HWND hStPrecioMes = GetDlgItem(hwnd, ST_PORMES);
		if (aux->formaPago != 1) {
			char precioMesC[20];
			snprintf(precioMesC, sizeof(precioMesC), "%.2f", precioMeses);
			SetWindowText(hStPrecioMes, precioMesC);
		}
		else {
			SetWindowText(hStPrecioMes, precioTotC);
		}
	}break;
	case WM_COMMAND:
		//OPCIONES DE PAGO DE CITA
		if (LOWORD(wParam) == ID_PC_CANCELA && HIWORD(wParam) == BN_CLICKED) {
			aux = origin;
			DestroyWindow(hPagarCita);
			hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
			ShowWindow(hAgenda, SW_SHOW);
			SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
		}
		if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED) {
			if (aux->next == NULL && aux->prev == NULL) {
				delete aux;
				aux = origin = NULL;
			}
			else if (aux->prev == NULL) {
				origin = origin->next;
				aux->next->prev = NULL;
				delete aux;
				aux = origin;
			}
			else if (aux->next == NULL) {
				aux->prev->next = NULL;
				delete aux;
				aux = origin;
			}
			else {
				aux->prev->next = aux->next;
				aux->next->prev = aux->prev;
				delete aux;
				aux = origin;
			}

			MessageBox(hwnd, "Se ha pagado la cita. Tenga un buen día.", "PAGADO", MB_OK);
			hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
			DestroyWindow(hPagarCita);
			ShowWindow(hAgenda, SW_SHOW);
			SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK editarInfoDoctor(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG: {
		hBarraMenu = GetMenu(hwnd);
		EnableMenuItem(hBarraMenu, BTN_EDITDOCINFO, MF_DISABLED);
		EnableMenuItem(hBarraMenu, BTN_AGENDA, MF_ENABLED);
		EnableMenuItem(hBarraMenu, BTN_NUEVACITA, MF_ENABLED);
		EnableMenuItem(hBarraMenu, BTN_SALIR, MF_ENABLED);

		HWND hEdNombreMedico = GetDlgItem(hwnd, EDT_EDM_DOCTOR);
		HWND hEdCedula = GetDlgItem(hwnd, EDT_EDM_CEDULA);
		SetWindowText(hEdNombreMedico, doc->nombreMedico);
		SetWindowText(hEdCedula, doc->cedula);
		hPcFotoDoctor = GetDlgItem(hwnd, BMP_EDM_DOCTOR);
		hBmpDoctor = (HBITMAP)LoadImage(NULL, doc->chDirFotoDoc, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
		SendMessage(hPcFotoDoctor, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpDoctor);
		strcpy(chCambioFoto, doc->chDirFotoDoc);
	}break;
	case WM_COMMAND: {
		//OPCIONES DE BARRA DE MENU
		if (LOWORD(wParam) == BTN_AGENDA && HIWORD(wParam) == BN_CLICKED) {
			strcpy(doc->chDirFotoDoc, chCambioFoto);
			DestroyWindow(hEditarDoctor);
			hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
			ShowWindow(hAgenda, SW_SHOW);
			SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
		}
		if (LOWORD(wParam) == BTN_NUEVACITA && HIWORD(wParam) == BN_CLICKED) {
			strcpy(doc->chDirFotoDoc, chCambioFoto);
			DestroyWindow(hEditarDoctor);
			hNuevaCita = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_NUEVACITA), NULL, nuevaCita);
			ShowWindow(hNuevaCita, SW_SHOW);
			SetTimer(hNuevaCita, TM_NC_RELOJ, 1000, NULL);
		}
		if (LOWORD(wParam) == BTN_SALIR && HIWORD(wParam) == BN_CLICKED) {
			strcpy(doc->chDirFotoDoc, chCambioFoto);
			if (MessageBox(hwnd, "¿Seguro que quiere salir del programa?", "SALIR", MB_YESNO) == IDYES) {
				salida = true;
				DestroyWindow(hEditarDoctor);
			}
			break;
		}
		//OPCIONES DE EDICIÓN DE PERFIL DEL DOCTOR
		if (LOWORD(wParam) == ID_EDM_CANCELA && HIWORD(wParam) == BN_CLICKED) {
			strcpy(doc->chDirFotoDoc, chCambioFoto);
			DestroyWindow(hEditarDoctor);
			hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
			ShowWindow(hAgenda, SW_SHOW);
			SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
			break;
		}
		if (LOWORD(wParam) == BTN_EDM_DOCTOR && HIWORD(wParam) == BN_CLICKED) {
			fotoDoctor(hwnd);
			break;
		}
		if (LOWORD(wParam) == BTN_EDM_BORRARFOTO && HIWORD(wParam) == BN_CLICKED) {
			strcpy(chCambioFoto, doc->chDirFotoDoc);
			borrarFotoDoctor(hwnd);
			break;
		}
		if (LOWORD(wParam = ID_EDM_OK && HIWORD(wParam) == BN_CLICKED)) {
			HWND hEdNombreMedico = GetDlgItem(hwnd, EDT_EDM_DOCTOR);
			int length = GetWindowTextLength(hEdNombreMedico);
			if (length > 0) {
				GetWindowText(hEdNombreMedico, doc->nombreMedico, length + 1);
				string m(doc->nombreMedico);
				if (verificarNum(m)) {
					MessageBox(hwnd, "El nombre del médico no debe contener números.", "AVISO", MB_ICONEXCLAMATION);
					break;
				}
			}
			else {
				MessageBox(hwnd, "Llene el nombre del médico.", "AVISO", MB_ICONEXCLAMATION);
				break;
			}

			HWND hEdCedula = GetDlgItem(hwnd, EDT_EDM_CEDULA);
			length = GetWindowTextLength(hEdCedula);
			if (length > 0) {
				GetWindowText(hEdCedula, doc->cedula, length + 1);
			}
			else {
				MessageBox(hwnd, "Llene la cédula.", "AVISO", MB_ICONEXCLAMATION);
				break;
			}

			DestroyWindow(hEditarDoctor);
			hAgenda = CreateDialog(hInstGlobal, MAKEINTRESOURCE(IDD_AGENDA), NULL, agendaVentanaPrincipal);
			ShowWindow(hAgenda, SW_SHOW);
			SetTimer(hAgenda, TM_RELOJ, 1000, NULL);
		}
	}break;
	case WM_DESTROY:
		if (salida) {
			PostQuitMessage(0);
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK primerDoctor(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		MessageBox(hwnd, "Hola, doctor. Ingrese sus datos para empezar su agenda.", "BIENVENIDO", MB_ICONINFORMATION);
		doc = new DOCTOR;
	break;
	case WM_COMMAND:
		if (LOWORD(wParam) == BTN_EDM_DOCTOR && HIWORD(wParam) == BN_CLICKED) {
			fotoDoctor(hwnd);
		}
		if (LOWORD(wParam) == BTN_EDM_BORRARFOTO && HIWORD(wParam) == BN_CLICKED) {
			borrarFotoDoctor(hwnd);
		}
		if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED) {
			HWND hEdNombreMedico = GetDlgItem(hwnd, EDT_EDM_DOCTOR);
			int length = GetWindowTextLength(hEdNombreMedico);
			if (length > 0) {
				GetWindowText(hEdNombreMedico, doc->nombreMedico, length + 1);
				string m(doc->nombreMedico);
				if (verificarNum(m)) {
					MessageBox(hwnd, "El nombre del médico no debe contener números.", "AVISO", MB_ICONEXCLAMATION);
					break;
				}
			}
			else {
				MessageBox(hwnd, "Llene el nombre del médico.", "AVISO", MB_ICONEXCLAMATION);
				break;
			}

			HWND hEdCedula = GetDlgItem(hwnd, EDT_EDM_CEDULA);
			length = GetWindowTextLength(hEdCedula);
			if (length > 0) {
				GetWindowText(hEdCedula, doc->cedula, length + 1);
			}
			else {
				MessageBox(hwnd, "Llene la cédula.", "AVISO", MB_ICONEXCLAMATION);
				break;
			}

			SetWindowText(hLblNombreMedico, doc->nombreMedico);
			SetWindowText(hLblCedula, doc->cedula);
			EndDialog(hwnd, 0);
		}
		break;
	}
	return FALSE;
}
#pragma endregion

#pragma region Funciones
//Archivos
void saveDoctor() {
	archivo.open("datosDoctor.bin", ios::out | ios::trunc | ios::binary);
	if (!archivo.is_open()) {
		MessageBox(NULL, "No se pudo guardar archivo del doctor.", "AVISO", MB_ICONERROR);
		return;
	}
	DOCTOR *temp = doc;
	archivo.write(reinterpret_cast<char *>(temp), sizeof(CITA));
	archivo.close();
}

void saveLista(CITA *origin) {
	archivo.open("listaCitas.bin", ios::out | ios::trunc | ios::binary);
	if (!archivo.is_open()) {
		MessageBox(NULL, "No se pudo guardar archivo de citas.", "AVISO", MB_ICONERROR);
		return;
	}
	CITA *temp = origin;
	while (temp != NULL) {
		archivo.write(reinterpret_cast<char*>(temp), sizeof(CITA));
		temp = temp->next;
	}
	archivo.close();
	return;
}

void loadDoctor() {
	archivo.open("datosDoctor.bin", ios::in | ios::ate | ios::binary);
	if (archivo) {
		if (!archivo.is_open()) {
			MessageBox(NULL, "No se pudo cargar el archivo del doctor.", "AVISO", MB_ICONERROR);
			return;
		}
		int size = archivo.tellg();
		if (size == 0) {
			return;
		}
		doc = new DOCTOR;
		DOCTOR *temp = new DOCTOR;
		archivo.seekg(0);
		archivo.read(reinterpret_cast<char*>(temp), sizeof(DOCTOR));
		strcpy(doc->nombreMedico, temp->nombreMedico);
		strcpy(doc->cedula, temp->cedula);
		strcpy(doc->chDirFotoDoc, temp->chDirFotoDoc);
		delete reinterpret_cast<char *>(temp);
	}
	archivo.close();
	
}

void loadLista() {
	archivo.open("listaCitas.bin", ios::in | ios::ate | ios::binary);
	if (archivo) {
		if (!archivo.is_open()) {
			MessageBox(NULL, "No se pudo cargar el archivo de citas.", "AVISO", MB_ICONERROR);
			return;
		}
		int size = archivo.tellg();
		if (size == 0) {
			return;
		}
		for (int i = 0; i < (size / sizeof(CITA)); i++) {
			if (origin == NULL) {
				origin = new CITA;
				aux = origin;
				aux->prev = NULL;
			}
			else {
				while (aux->next != NULL)
					aux = aux->next;
				aux->next = new CITA;
				aux->next->prev = aux;
				aux = aux->next;
			}
			CITA *temp = new CITA;
			archivo.seekg(i * sizeof(CITA));
			archivo.read(reinterpret_cast<char*>(temp), sizeof(CITA));
			strcpy(aux->nombreDueño, temp->nombreDueño);
			strcpy(aux->nombreMascota, temp->nombreMascota);
			aux->telefono = temp->telefono;
			aux->especie = temp->especie;
			strcpy(aux->motivoConsulta, temp->motivoConsulta); 
			strcpy(aux->image1, temp->image1);
			strcpy(aux->image2, temp->image2);
			aux->fechaString = temp->fechaString;
			aux->horaString = temp->horaString;
			aux->year = temp->year;
			aux->month = temp->month;
			aux->day = temp->day;
			aux->hour = temp->hour;
			aux->minutes = temp->minutes;
			aux->especieIndex = temp->especieIndex;
			aux->formaPago = temp->formaPago;
			aux->costo = temp->costo;
			delete reinterpret_cast<char*>(temp);
			aux->next = NULL;
			aux = origin;
		}
	}
	archivo.close();
}

//Operaciones
bool verificarNum(string c) {
	bool r = false;
	int n = c.size();
	for (int i = 0; i < n; i++) {
		if (c[i] >= 48 && c[i] <= 57) {
			r = true;
			break;
		}
	}
	return r;
}

bool verificarAlfa(string c) {
	bool r = false;
	int n = c.size();
	int dots = 0;
	for (int i = 0; i < n; i++) {
		if (c[i] != 46 && c[i] < 47 || c[i] > 58) {
			r = true;
			return r;
		}
		if (c[i] == 46) {
			dots++;
			if (dots == 2) {
				r = true;
				return r;
			}
		}
	}
	return r;
}

bool introducirDatos(HWND hwnd) {
	bool r = false;
	char buff[256];
	int length;

	HWND hEdNombreM = GetDlgItem(hwnd, EDT_NC_MNOMBRE);
	length = GetWindowTextLength(hEdNombreM);
	if (length > 0) {
		GetWindowText(hEdNombreM, buff, length + 1);
		string s(buff);
		if (verificarNum(s)) {
			MessageBox(hwnd, "El nombre de la mascota no deben contener números.", "AVISO", MB_ICONEXCLAMATION);
			return r;
		}
		else {
			strcpy(aux->nombreMascota, buff);
		}
	}
	else {
		MessageBox(hwnd, "Falta llenar el nombre de la mascota.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}

	HWND hCbEspecie = GetDlgItem(hwnd, CB_NC_ESPECIE);
	int indice = SendMessage(hCbEspecie, CB_GETCURSEL, 0, 0);
	if (indice == -1) {
		MessageBox(hwnd, "Seleccione la especie de su mascota.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}
	else {
		length = GetWindowTextLength(hCbEspecie);
		GetWindowText(hCbEspecie, buff, length + 1);
		string s(buff);
		aux->especie = s;
		aux->especieIndex = indice;
	}

	HWND hEdNombreD = GetDlgItem(hwnd, EDT_NC_DNOMBRE);
	length = GetWindowTextLength(hEdNombreD);
	if (length > 0) {
		GetWindowText(hEdNombreD, buff, length + 1);
		string s(buff);
		if (verificarNum(s)) {
			MessageBox(hwnd, "El nombre de dueño no deben contener números.", "AVISO", MB_ICONEXCLAMATION);
			return r;
		}
		else {
			strcpy(aux->nombreDueño, buff);
		}
	}
	else {
		MessageBox(hwnd, "Falta llenar el nombre del dueño.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}

	HWND hEdTel = GetDlgItem(hwnd, EDT_NC_TELEFONO);
	length = GetWindowTextLength(hEdTel);
	if (length > 0) {
		if (length == 8 || length == 10 || length == 12) {
			GetWindowText(hEdTel, buff, length + 1);
			string s(buff);
			aux->telefono = s;
		}
		else {
			MessageBox(hwnd, "El teléfono debe ser de 8, 10 o 12 caracteres.", "AVISO", MB_ICONEXCLAMATION);
			return r;
		}
	}
	else {
		MessageBox(hwnd, "Falta llenar el teléfono.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}

	HWND hTpFecha = GetDlgItem(hwnd, IDC_DATETIMEPICKER1);
	length = GetWindowTextLength(hTpFecha);
	GetWindowText(hTpFecha, buff, length + 4);
	string f(buff);
	string Y = f.substr(6, 4);
	string m = f.substr(3, 2);
	string d = f.substr(0, 2);
	HWND hTpHora = GetDlgItem(hwnd, IDC_DATETIMEPICKER2);
	GetWindowText(hTpHora, buff, 15);
	string h(buff);
	string I = h.substr(0, 2);
	string min = h.substr(3, 2);
	string amPm = h.substr(9, 1);
	string stdHour;
	//VALIDCIÓN DE FECHA NO ANTIGUA
	int yearComp = stoi(Y);
	int monthComp = stoi(m);
	int dayComp = stoi(d);
	int hourComp = stoi(I);
	int minuteComp = stoi(min);
	if (yearComp >= tiempoActual->tm_year + 1900) {
		if (yearComp == tiempoActual->tm_year + 1900) {
			if (monthComp >= tiempoActual->tm_mon + 1) {
				if (monthComp == tiempoActual->tm_mon + 1) {
					if (dayComp >= tiempoActual->tm_mday) {
						if (dayComp == tiempoActual->tm_mday) {
							if (amPm == "p") {
								if (hourComp != 12) {
									hourComp += 12;
								}
							}
							if (amPm == "a" && hourComp == 12) {
								hourComp -= 12;
							}
							if (hourComp >= tiempoActual->tm_hour) {
								if (hourComp == tiempoActual->tm_hour) {
									if (minuteComp > tiempoActual->tm_min) {
										aux->year = yearComp;
										aux->month = monthComp;
										aux->day = dayComp;
										aux->hour = hourComp;
										aux->minutes = minuteComp;
									}
									else {
										MessageBox(hwnd, "La hora no debe ser igual o pasada al tiempo actual.", "AVISO", MB_ICONEXCLAMATION);
										return r;
									}
								}
								else {
									aux->year = yearComp;
									aux->month = monthComp;
									aux->day = dayComp;
									aux->hour = hourComp;
									aux->minutes = minuteComp;
								}
							}
							else {
								MessageBox(hwnd, "La hora no debe ser igual o pasada al tiempo actual.", "AVISO", MB_ICONEXCLAMATION);
								return r;
							}
						}
						else {
							if (amPm == "p") {
								if (hourComp != 12) {
									hourComp += 12;
								}
							}
							if (amPm == "a" && hourComp == 12) {
								hourComp -= 12;
							}
							aux->year = yearComp;
							aux->month = monthComp;
							aux->day = dayComp;
							aux->hour = hourComp;
							aux->minutes = minuteComp;
						}
					}
					else {
						MessageBox(hwnd, "Coloque fecha válida, el día debe ser actual o próximo.", "AVISO", MB_ICONEXCLAMATION);
						return r;
					}
				}
				else {
					if (amPm == "p") {
						if (hourComp != 12) {
							hourComp += 12;
						}
					}
					if (amPm == "a" && hourComp == 12) {
						hourComp -= 12;
					}
					aux->year = yearComp;
					aux->month = monthComp;
					aux->day = dayComp;
					aux->hour = hourComp;
					aux->minutes = minuteComp;
				}
			}
			else {
				MessageBox(hwnd, "Coloque fecha válida, el mes debe ser actual o próximo.", "AVISO", MB_ICONEXCLAMATION);
				return r;
			}
		}
		else {
			if (amPm == "p") {
				if (hourComp != 12) {
					hourComp += 12;
				}
			}
			if (amPm == "a" && hourComp == 12) {
				hourComp -= 12;
			}
			aux->year = yearComp;
			aux->month = monthComp;
			aux->day = dayComp;
			aux->hour = hourComp;
			aux->minutes = minuteComp;
		}
	}
	else {
		MessageBox(hwnd, "Coloque fecha válida, el año debe ser actual o próximo.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}
	//VALIDACIÓN DE FECHA NO REPETIDA
	aux->fechaString = "";
	aux->horaString = "";
	if (amPm == "p") {
		stdHour = I + ":" + min + " P.M.";
	}
	else {
		stdHour = I + ":" + min + " A.M.";
	}
	CITA *auxCF = origin;
	bool fechaRepetida = false;
	while (auxCF != NULL) {
		if (f.compare(auxCF->fechaString) == 0) {
			if (stdHour.compare(auxCF->horaString) == 0) {
				fechaRepetida = true;
				break;
			}
		}
		auxCF = auxCF->next;
	}
	if (fechaRepetida) {
		MessageBox(hwnd, "La fecha y hora de la cita ya no está disponible. Ya existe otra cita en este horario.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}
	else {
		aux->fechaString = f;
		aux->horaString = stdHour;
	}

	HWND hEdMotivo = GetDlgItem(hwnd, EDT_NC_MOTIVO);
	length = GetWindowTextLength(hEdMotivo);
	if (length > 0) {
		GetWindowText(hEdMotivo, buff, length + 1);
		strcpy(aux->motivoConsulta, buff);
	}
	else {
		MessageBox(hwnd, "Especifique el motivo de la consulta.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}

	HWND hEdCosto = GetDlgItem(hwnd, EDT_NC_COSTO);
	length = GetWindowTextLength(hEdCosto);
	if (length > 0) {
		GetWindowText(hEdCosto, buff, length + 1);
		string s(buff);
		if (verificarAlfa(s)) {
			MessageBox(hwnd, "El costo no debe contener letras o más de un punto decimal. Corrija.", "AVISO", MB_ICONEXCLAMATION);
			return r;
		}
		else {
			aux->costo = atof(buff);
		}
	}
	else {
		MessageBox(hwnd, "Agregue el costo de la consulta.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}

	if (IsDlgButtonChecked(hwnd, RD_NC_CONTADO) == BST_CHECKED) {
		aux->formaPago = 1;
	}
	else if (IsDlgButtonChecked(hwnd, RD_NC_3MESES) == BST_CHECKED) {
		aux->formaPago = 3;
	}
	else if (IsDlgButtonChecked(hwnd, RD_NC_6MESES) == BST_CHECKED) {
		aux->formaPago = 6;
	}
	else if (IsDlgButtonChecked(hwnd, RD_NC_9MESES) == BST_CHECKED) {
		aux->formaPago = 9;
	}
	else {
		MessageBox(hwnd, "Seleccione una forma de pago.", "AVISO", MB_ICONEXCLAMATION);
		return r;
	}

	r = true;
	return r;
}

void ordenamiento() {
	CITA *auxActual, *auxProx;
	auxActual = origin;
	while (auxActual != NULL) {
		auxProx = auxActual->next;
		while (auxProx != NULL) {
			if (auxActual->year > auxProx->year) {
				intercambio(auxActual, auxProx);
			}
			if (auxActual->year == auxProx->year && auxActual->month > auxProx->month) {
				intercambio(auxActual, auxProx);
			}
			if (auxActual->year == auxProx->year && auxActual->month == auxProx->month && auxActual->day > auxProx->day) {
				intercambio(auxActual, auxProx);
			}
			if (auxActual->year == auxProx->year && auxActual->month == auxProx->month && auxActual->day == auxProx->day && auxActual->hour > auxProx->hour) {
				intercambio(auxActual, auxProx);
			}
			if (auxActual->year == auxProx->year && auxActual->month == auxProx->month && auxActual->day == auxProx->day && auxActual->hour == auxProx->hour && auxActual->minutes > auxProx->minutes) {
				intercambio(auxActual, auxProx);
			}
			auxProx = auxProx->next;
		}
		auxActual = auxActual->next;
	}
}

void intercambio(CITA *auxActual, CITA *auxProx) {
	char tNombreD[50];
	strcpy(tNombreD, auxProx->nombreDueño);
	char tNombreM[50];
	strcpy(tNombreM, auxProx->nombreMascota);
	string tTel = auxProx->telefono;
	string tEsp = auxProx->especie;
	char tMotivo[MAX_PATH];
	strcpy(tMotivo, aux->motivoConsulta);
	char tImg1[MAX_PATH];
	strcpy(tImg1, auxProx->image1);
	char tImg2[MAX_PATH];
	strcpy(tImg2, auxProx->image2);
	string tFechaStr = auxProx->fechaString;
	string tHoraStr = auxProx->horaString;
	int tY = auxProx->year;
	int tM = auxProx->month;
	int tD = auxProx->day;
	int tH = auxProx->hour;
	int tMin = auxProx->minutes;
	int tEspInd = auxProx->especieIndex;
	int tPago = auxProx->formaPago;
	float tCosto = auxProx->costo;

	strcpy(auxProx->nombreDueño, auxActual->nombreDueño);
	strcpy(auxProx->nombreMascota, auxActual->nombreMascota);
	auxProx->telefono = auxActual->telefono;
	auxProx->especie = auxActual->especie;
	strcpy(auxProx->motivoConsulta, auxActual->motivoConsulta);
	strcpy(auxProx->image1, auxActual->image1); 
	strcpy(auxProx->image2, auxActual->image2);
	auxProx->fechaString = auxActual->fechaString;
	auxProx->horaString = auxActual->horaString;
	auxProx->year = auxActual->year;
	auxProx->month = auxActual->month;
	auxProx->day = auxActual->day;
	auxProx->hour = auxActual->hour;
	auxProx->minutes = auxActual->minutes;
	auxProx->especieIndex = auxActual->especieIndex;
	auxProx->formaPago = auxActual->formaPago;
	auxProx->costo = auxActual->costo;

	strcpy(auxActual->nombreDueño, tNombreD);
	strcpy(auxActual->nombreMascota, tNombreM);
	auxActual->telefono = tTel;
	auxActual->especie = tEsp;
	strcpy(auxActual->motivoConsulta, tMotivo);
	strcpy(auxActual->image1, tImg1);
	strcpy(auxActual->image2, tImg2);
	auxActual->fechaString = tFechaStr;
	auxActual->horaString = tHoraStr;
	auxActual->year = tY;
	auxActual->month = tM;
	auxActual->day = tD;
	auxActual->hour = tH;
	auxActual->minutes = tMin;
	auxActual->especieIndex = tEspInd;
	auxActual->formaPago = tPago;
	auxActual->costo = tCosto;
}

void impresion() {
	aux = origin;
	while (aux != NULL) {
		string mascota(aux->nombreMascota);
		string display = mascota + "  |  " + aux->fechaString + "  |  " + aux->horaString;
		char buffL[100];
		strcpy(buffL, display.c_str());
		SendMessage(hLbAgenda, LB_ADDSTRING, 0, (LPARAM)buffL);

		aux = aux->next;
	}
	aux = origin;
}

void limpiarMostrador(HWND hwnd) {
	HWND hLblListCount = GetDlgItem(hwnd, ST_LISTCOUNT);
	int lista = SendMessage(hLbAgenda, LB_GETCOUNT, 0, 0);
	char listaC[20];
	_itoa(lista, listaC, 10);
	SetWindowText(hLblListCount, listaC);

	HWND hBtnEdit = GetDlgItem(hwnd, BTN_EDITARCITA);
	HWND hBtnPagar = GetDlgItem(hwnd, BTN_PAGARCITA);
	HWND hBtnEliminar = GetDlgItem(hwnd, BTN_ELIMINARCITA);
	EnableWindow(hBtnEdit, false);
	EnableWindow(hBtnPagar, false);
	EnableWindow(hBtnEliminar, false);

	HWND hStMascota = GetDlgItem(hwnd, ST_INFO_MASCOTA);
	SetWindowText(hStMascota, "");
	HWND hStEspecie = GetDlgItem(hwnd, ST_INFO_ESPECIE);
	SetWindowText(hStEspecie, "");
	HWND hStDueno = GetDlgItem(hwnd, ST_INFO_DUENO);
	SetWindowText(hStDueno, "");
	HWND hStTelefono = GetDlgItem(hwnd, ST_INFO_TELEFONO);
	SetWindowText(hStTelefono, "");
	HWND hStFechaHora = GetDlgItem(hwnd, ST_INFO_FECHA);
	SetWindowText(hStFechaHora, "");
	HWND hStCosto = GetDlgItem(hwnd, ST_INFO_COSTO);
	SetWindowText(hStCosto, "");
	HWND hStMotivo = GetDlgItem(hwnd, ST_INFO_MOTIVO);
	SetWindowText(hStMotivo, "");
	strcpy(chCambioFoto, "");
	hPcFotoMascota = GetDlgItem(hwnd, BMP_MENU_MASCOTA1);
	hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 90, 108, LR_LOADFROMFILE);
	SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
	hPcFotoMascota = GetDlgItem(hwnd, BMP_MENU_MASCOTA2);
	hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 90, 108, LR_LOADFROMFILE);
	SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
}

void fotoDoctor(HWND hwnd) {
	strcpy(chCambioFoto, doc->chDirFotoDoc);
	OPENFILENAME ofnFotoDoctor;
	ZeroMemory(&ofnFotoDoctor, sizeof(ofnFotoDoctor));

	ofnFotoDoctor.hwndOwner = hwnd;
	ofnFotoDoctor.lStructSize = sizeof(ofnFotoDoctor);
	ofnFotoDoctor.lpstrFile = doc->chDirFotoDoc;
	ofnFotoDoctor.nMaxFile = MAX_PATH;
	ofnFotoDoctor.lpstrDefExt = "bmp";
	ofnFotoDoctor.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	ofnFotoDoctor.lpstrFilter = "BMP Images\0*.bmp\0All Files\0*.*\0";
	if (GetOpenFileName(&ofnFotoDoctor)) {
		hPcFotoDoctor = GetDlgItem(hwnd, BMP_EDM_DOCTOR);
		hBmpDoctor = (HBITMAP)LoadImage(NULL, doc->chDirFotoDoc, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
		SendMessage(hPcFotoDoctor, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpDoctor);
	}
}

void borrarFotoMascota(HWND hwnd) {
	if (indexImage == 0) {
		strcpy(aux->image1, "");
	}
	if (indexImage == 1) {
		strcpy(aux->image2, "");
	}
	strcpy(chCambioFoto, "");
	hPcFotoMascota = GetDlgItem(hwnd, BMP_NC_MASCOTA);
	hBmpMascota = (HBITMAP)LoadImage(NULL, chCambioFoto, IMAGE_BITMAP, 95, 114, LR_LOADFROMFILE);
	SendMessage(hPcFotoMascota, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpMascota);
}

void borrarFotoDoctor(HWND hwnd) {
	strcpy(chCambioFoto, doc->chDirFotoDoc);
	strcpy(doc->chDirFotoDoc, "");
	hPcFotoDoctor = GetDlgItem(hwnd, BMP_EDM_DOCTOR);
	hBmpDoctor = (HBITMAP)LoadImage(NULL, doc->chDirFotoDoc, IMAGE_BITMAP, 100, 120, LR_LOADFROMFILE);
	SendMessage(hPcFotoDoctor, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpDoctor);
}
#pragma endregion